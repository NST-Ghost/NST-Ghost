#include "qtlingo/mcpclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QHash>
#include <QProcess>

namespace qtlingo {

namespace {

constexpr auto kProtocolVersion = "2025-06-18";

#ifndef APP_VERSION
#define APP_VERSION "0.0.0"
#endif

QJsonObject environmentToJson(const QProcessEnvironment &environment)
{
    QJsonObject object;
    const QStringList keys = environment.keys();
    for (const QString &key : keys) {
        object[key] = environment.value(key);
    }
    return object;
}

QProcessEnvironment environmentFromVariant(const QVariant &value)
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QVariantMap map = value.toMap();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        environment.insert(it.key(), it.value().toString());
    }
    return environment;
}

QStringList stringListFromVariant(const QVariant &value)
{
    QStringList list = value.toStringList();
    if (!list.isEmpty()) {
        return list;
    }

    const QVariantList variantList = value.toList();
    for (const QVariant &item : variantList) {
        list.append(item.toString());
    }
    return list;
}

} // namespace

McpServerConfig McpServerConfig::fromVariantMap(const QVariantMap &settings)
{
    McpServerConfig config;
    config.name = settings.value("name").toString();
    config.command = settings.value("command").toString();
    config.arguments = stringListFromVariant(settings.value("args"));
    if (config.arguments.isEmpty()) {
        config.arguments = stringListFromVariant(settings.value("arguments"));
    }
    config.workingDirectory = settings.value("workingDirectory").toString();
    config.environment = environmentFromVariant(settings.value("env"));
    config.enabled = settings.value("enabled", true).toBool();
    return config;
}

QVariantMap McpServerConfig::toVariantMap() const
{
    QVariantMap settings;
    settings["name"] = name;
    settings["command"] = command;
    settings["args"] = arguments;
    settings["workingDirectory"] = workingDirectory;
    settings["env"] = environmentToJson(environment).toVariantMap();
    settings["enabled"] = enabled;
    return settings;
}

McpClient::McpClient(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::started, this, &McpClient::onStarted);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &McpClient::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &McpClient::onReadyReadStandardError);
    connect(m_process, &QProcess::errorOccurred, this, &McpClient::onProcessError);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) { onFinished(exitCode); });
}

McpClient::~McpClient()
{
    stop();
}

bool McpClient::start(const McpServerConfig &config)
{
    if (!config.enabled) {
        emit disconnected(-1, "MCP server is disabled.");
        return false;
    }
    if (config.command.trimmed().isEmpty()) {
        emit disconnected(-1, "MCP server command is empty.");
        return false;
    }

    stop();

    m_config = config;
    m_stdoutBuffer.clear();
    m_pendingRequests.clear();
    m_nextRequestId = 1;
    m_initialized = false;

    if (!m_config.workingDirectory.isEmpty()) {
        m_process->setWorkingDirectory(m_config.workingDirectory);
    }
    if (!m_config.environment.isEmpty()) {
        m_process->setProcessEnvironment(m_config.environment);
    }

    m_process->setProgram(m_config.command);
    m_process->setArguments(m_config.arguments);
    m_process->start();
    return true;
}

void McpClient::stop(int timeoutMs)
{
    if (m_process->state() == QProcess::NotRunning) {
        return;
    }

    m_process->closeWriteChannel();
    if (!m_process->waitForFinished(timeoutMs)) {
        m_process->terminate();
        if (!m_process->waitForFinished(timeoutMs)) {
            m_process->kill();
            m_process->waitForFinished(timeoutMs);
        }
    }
}

bool McpClient::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

bool McpClient::isInitialized() const
{
    return m_initialized;
}

McpServerConfig McpClient::config() const
{
    return m_config;
}

int McpClient::listTools(const QString &cursor)
{
    QJsonObject params;
    if (!cursor.isEmpty()) {
        params["cursor"] = cursor;
    }
    return sendRequest("tools/list", params);
}

int McpClient::callTool(const QString &toolName, const QJsonObject &arguments)
{
    QJsonObject params;
    params["name"] = toolName;
    params["arguments"] = arguments;
    return sendRequest("tools/call", params);
}

int McpClient::sendRequest(const QString &method, const QJsonObject &params)
{
    const int id = m_nextRequestId++;

    PendingRequest pending;
    pending.method = method;
    if (method == "tools/call") {
        pending.toolName = params.value("name").toString();
    }
    m_pendingRequests.insert(id, pending);

    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = id;
    message["method"] = method;
    if (!params.isEmpty()) {
        message["params"] = params;
    }
    writeMessage(message);

    return id;
}

void McpClient::sendNotification(const QString &method, const QJsonObject &params)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["method"] = method;
    if (!params.isEmpty()) {
        message["params"] = params;
    }
    writeMessage(message);
}

void McpClient::sendResponse(int id, const QJsonObject &result)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = id;
    message["result"] = result;
    writeMessage(message);
}

void McpClient::sendError(int id, int code, const QString &messageText, const QJsonObject &data)
{
    QJsonObject error;
    error["code"] = code;
    error["message"] = messageText;
    if (!data.isEmpty()) {
        error["data"] = data;
    }

    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = id;
    message["error"] = error;
    writeMessage(message);
}

void McpClient::onStarted()
{
    QJsonObject clientInfo;
    clientInfo["name"] = "NST";
    clientInfo["version"] = APP_VERSION;

    QJsonObject params;
    params["protocolVersion"] = kProtocolVersion;
    params["capabilities"] = QJsonObject();
    params["clientInfo"] = clientInfo;

    sendRequest("initialize", params);
}

void McpClient::onReadyReadStandardOutput()
{
    m_stdoutBuffer.append(m_process->readAllStandardOutput());

    qsizetype newlineIndex = m_stdoutBuffer.indexOf('\n');
    while (newlineIndex != -1) {
        const QByteArray line = m_stdoutBuffer.left(newlineIndex).trimmed();
        m_stdoutBuffer.remove(0, newlineIndex + 1);
        if (!line.isEmpty()) {
            processLine(line);
        }
        newlineIndex = m_stdoutBuffer.indexOf('\n');
    }
}

void McpClient::onReadyReadStandardError()
{
    const QString log = QString::fromUtf8(m_process->readAllStandardError()).trimmed();
    if (!log.isEmpty()) {
        emit serverLog(log);
    }
}

void McpClient::onProcessError()
{
    emit disconnected(-1, m_process->errorString());
}

void McpClient::onFinished(int exitCode)
{
    m_initialized = false;
    emit disconnected(exitCode, m_process->errorString());
}

void McpClient::writeMessage(const QJsonObject &message)
{
    if (m_process->state() == QProcess::NotRunning) {
        emit disconnected(-1, "MCP server process is not running.");
        return;
    }

    QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    payload.append('\n');
    m_process->write(payload);
}

void McpClient::processLine(const QByteArray &line)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit requestFailed(-1, QString(), QString("Invalid MCP message: %1").arg(parseError.errorString()), QJsonObject());
        return;
    }

    const QJsonObject message = document.object();
    if (message.contains("method") && message.contains("id")) {
        emit protocolRequest(message.value("id").toInt(-1),
                             message.value("method").toString(),
                             message.value("params").toObject());
    } else if (message.contains("method")) {
        handleNotification(message);
    } else if (message.contains("id")) {
        handleResponse(message);
    }
}

void McpClient::handleResponse(const QJsonObject &message)
{
    const int id = message.value("id").toInt(-1);
    const PendingRequest pending = m_pendingRequests.take(id);

    if (message.contains("error")) {
        const QJsonObject error = message.value("error").toObject();
        emit requestFailed(id, pending.method, error.value("message").toString("MCP request failed."), error);
        return;
    }

    const QJsonObject result = message.value("result").toObject();

    if (pending.method == "initialize") {
        m_initialized = true;
        emit initialized(result.value("serverInfo").toObject(), result.value("capabilities").toObject());
        sendNotification("notifications/initialized");
    } else if (pending.method == "tools/list") {
        emit toolsListed(parseTools(result), result.value("nextCursor").toString());
    } else if (pending.method == "tools/call") {
        emit toolCallFinished(pending.toolName, result);
    }

    emit requestFinished(id, pending.method, result);
}

void McpClient::handleNotification(const QJsonObject &message)
{
    emit protocolNotification(message.value("method").toString(), message.value("params").toObject());
}

QList<McpTool> McpClient::parseTools(const QJsonObject &result) const
{
    QList<McpTool> tools;
    const QJsonArray toolArray = result.value("tools").toArray();
    for (const QJsonValue &value : toolArray) {
        const QJsonObject object = value.toObject();
        McpTool tool;
        tool.serverName = m_config.name;
        tool.name = object.value("name").toString();
        tool.title = object.value("title").toString();
        tool.description = object.value("description").toString();
        tool.inputSchema = object.value("inputSchema").toObject();
        tools.append(tool);
    }
    return tools;
}

} // namespace qtlingo
