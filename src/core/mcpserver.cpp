#include "mcpserver.h"
#include <qtlingo/mcplocaltransport.h>
#include <iostream>
#include <string>
#include <thread>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QDateTime>
#include <QTimer>

McpServer::McpServer(TranslationCore *core, QObject *parent)
    : QObject(parent)
    , m_core(core)
{
    // Connect to core signals to keep track of state and build a nice log
    connect(m_core, &TranslationCore::projectLoaded, this, [this](const QString &path) {
        m_logs.append(QString("[%1] Project loaded: %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), path));
    });

    connect(m_core, &TranslationCore::translationStarted, this, [this]() {
        m_logs.append(QString("[%1] Translation pipeline started.").arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
    });

    connect(m_core, &TranslationCore::translationFinished, this, [this]() {
        m_logs.append(QString("[%1] Translation pipeline finished.").arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
    });

    connect(m_core, &TranslationCore::totalProgressUpdated, this, [this](int processed, int total) {
        m_logs.append(QString("[%1] Progress updated: %2/%3 texts processed.").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), QString::number(processed), QString::number(total)));
    });

    connect(m_core, &TranslationCore::errorOccurred, this, [this](const QString &msg) {
        m_logs.append(QString("[%1] ERROR: %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), msg));
    });
}

McpServer::~McpServer()
{
    stop();
}

// ─── Stdio Transport ────────────────────────────────────────────

void McpServer::start()
{
    m_transportMode = Stdio;

    // Spawn standard input reader thread
    std::thread([this]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            QString qline = QString::fromStdString(line).trimmed();
            if (qline.isEmpty()) continue;

            // Safely execute request on the main GUI/Qt thread
            QMetaObject::invokeMethod(this, [this, qline]() {
                handleStdioLine(qline);
            }, Qt::QueuedConnection);
        }
        std::cerr << "[MCP Server] stdin EOF reached. Exiting..." << std::endl;
        QMetaObject::invokeMethod(qApp, &QCoreApplication::quit, Qt::QueuedConnection);
    }).detach();
}

void McpServer::handleStdioLine(const QString &line)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(line.toUtf8(), &parseError);
    if (document.isNull() || !document.isObject()) {
        std::cerr << "MCP Server parse error: " << parseError.errorString().toStdString() << std::endl;
        return;
    }

    QJsonObject request = document.object();
    QJsonObject response = handleRequest(request);

    if (!response.isEmpty()) {
        writeStdioMessage(response);
    }
}

void McpServer::writeStdioMessage(const QJsonObject &message)
{
    QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    std::cout << payload.constData() << std::endl;
}

// ─── Local Socket Transport ─────────────────────────────────────

bool McpServer::startLocal(const QString &socketPath)
{
    m_transportMode = Local;

    if (!m_localTransport) {
        m_localTransport = new qtlingo::McpLocalTransport(this);

        connect(m_localTransport, &qtlingo::McpLocalTransport::messageReceived,
                this, &McpServer::handleLocalMessage);

        connect(m_localTransport, &qtlingo::McpLocalTransport::clientConnected,
                this, [this](quintptr) {
            emit clientConnected(m_localTransport->connectedClientCount());
        });

        connect(m_localTransport, &qtlingo::McpLocalTransport::clientDisconnected,
                this, [this](quintptr) {
            emit clientDisconnected(m_localTransport->connectedClientCount());
        });

        connect(m_localTransport, &qtlingo::McpLocalTransport::serverError,
                this, [](const QString &error) {
            std::cerr << "[MCP Server] Transport error: " << error.toStdString() << std::endl;
        });
    }

    bool ok = m_localTransport->start(socketPath);
    if (ok) {
        std::cerr << "[MCP Server] Embedded MCP server started on: "
                  << m_localTransport->socketPath().toStdString() << std::endl;
    }
    return ok;
}

void McpServer::stop()
{
    if (m_localTransport) {
        m_localTransport->stop();
    }
}

bool McpServer::isListening() const
{
    return m_localTransport && m_localTransport->isListening();
}

QString McpServer::socketPath() const
{
    return m_localTransport ? m_localTransport->socketPath() : QString();
}

McpServer::TransportMode McpServer::transportMode() const
{
    return m_transportMode;
}

int McpServer::connectedClientCount() const
{
    return m_localTransport ? m_localTransport->connectedClientCount() : 0;
}

void McpServer::handleLocalMessage(quintptr clientId, const QJsonObject &message)
{
    QJsonObject response = handleRequest(message);
    if (!response.isEmpty() && m_localTransport) {
        m_localTransport->sendMessage(clientId, response);
    }
}

// ─── Transport-Agnostic Request Handler ─────────────────────────

QJsonObject McpServer::handleRequest(const QJsonObject &request)
{
    QString method = request["method"].toString();
    int id = request["id"].toInt(-1);

    if (method == "initialize") {
        return handleInitialize(request);
    }
    else if (method == "initialized") {
        // Handshake finished, no reply required
        return QJsonObject();
    }
    else if (method == "tools/list") {
        QJsonObject result = handleToolsList();
        return makeResponse(id, result);
    }
    else if (method == "tools/call") {
        QJsonObject params = request["params"].toObject();
        QJsonObject result = handleToolCall(params);
        return makeResponse(id, result);
    }
    else {
        if (id != -1) {
            return makeError(id, -32601, QString("Method not found: %1").arg(method));
        }
        return QJsonObject();
    }
}

QJsonObject McpServer::handleInitialize(const QJsonObject &request)
{
    int id = request["id"].toInt(-1);

    QJsonObject result;
    result["protocolVersion"] = "2024-11-05";

    QJsonObject capabilities;
    capabilities["tools"] = QJsonObject();
    result["capabilities"] = capabilities;

    QJsonObject serverInfo;
    serverInfo["name"] = "nst-server";
    serverInfo["version"] = "1.0.0";
    result["serverInfo"] = serverInfo;

    return makeResponse(id, result);
}

QJsonObject McpServer::handleToolsList()
{
    QJsonObject result;
    QJsonArray tools;

    // 1. nst_load_project
    {
        QJsonObject tool;
        tool["name"] = "nst_load_project";
        tool["description"] = "Load a game project into the translation engine and extract its strings";

        QJsonObject inputSchema;
        inputSchema["type"] = "object";

        QJsonObject properties;
        {
            QJsonObject engine;
            engine["type"] = "string";
            engine["description"] = "Game engine: rpgm, renpy, or unity";
            properties["engine"] = engine;

            QJsonObject path;
            path["type"] = "string";
            path["description"] = "Absolute path to the game directory";
            properties["path"] = path;
        }
        inputSchema["properties"] = properties;

        QJsonArray required;
        required.append("engine");
        required.append("path");
        inputSchema["required"] = required;

        tool["inputSchema"] = inputSchema;
        tools.append(tool);
    }

    // 2. nst_start_translation
    {
        QJsonObject tool;
        tool["name"] = "nst_start_translation";
        tool["description"] = "Start the translation process using the selected translation service";

        QJsonObject inputSchema;
        inputSchema["type"] = "object";

        QJsonObject properties;
        {
            QJsonObject serviceName;
            serviceName["type"] = "string";
            serviceName["description"] = "Translation service name (e.g. 'Google Translate' or a custom Lua plugin like 'Groq Translator'). Leave blank to use default.";
            properties["serviceName"] = serviceName;
        }
        inputSchema["properties"] = properties;
        tool["inputSchema"] = inputSchema;
        tools.append(tool);
    }

    // 3. nst_get_status
    {
        QJsonObject tool;
        tool["name"] = "nst_get_status";
        tool["description"] = "Get current status, total/processed texts, translation progress, and recent logs";
        tool["inputSchema"] = QJsonObject();
        tools.append(tool);
    }

    // 4. nst_deploy_project
    {
        QJsonObject tool;
        tool["name"] = "nst_deploy_project";
        tool["description"] = "Deploy the translated text back into the game files";

        QJsonObject inputSchema;
        inputSchema["type"] = "object";

        QJsonObject properties;
        {
            QJsonObject targetDir;
            targetDir["type"] = "string";
            targetDir["description"] = "Target folder for deployment. Leave blank to overwrite the loaded game folder directly.";
            properties["targetDir"] = targetDir;

            QJsonObject createBackup;
            createBackup["type"] = "boolean";
            createBackup["description"] = "Whether to create a backup in '_nst_backup' directory. Default: true.";
            properties["createBackup"] = createBackup;
        }
        inputSchema["properties"] = properties;
        tool["inputSchema"] = inputSchema;
        tools.append(tool);
    }

    // 5. nst_save_workspace
    {
        QJsonObject tool;
        tool["name"] = "nst_save_workspace";
        tool["description"] = "Save the translation database workspace to a .nst file";

        QJsonObject inputSchema;
        inputSchema["type"] = "object";

        QJsonObject properties;
        {
            QJsonObject filePath;
            filePath["type"] = "string";
            filePath["description"] = "Absolute path where to save the .nst file.";
            properties["filePath"] = filePath;
        }
        inputSchema["properties"] = properties;

        QJsonArray required;
        required.append("filePath");
        inputSchema["required"] = required;

        tool["inputSchema"] = inputSchema;
        tools.append(tool);
    }

    result["tools"] = tools;
    return result;
}

QJsonObject McpServer::handleToolCall(const QJsonObject &params)
{
    QString name = params["name"].toString();
    QJsonObject args = params["arguments"].toObject();

    if (name == "nst_load_project") {
        return handleLoadProject(args);
    }
    else if (name == "nst_start_translation") {
        return handleStartTranslation(args);
    }
    else if (name == "nst_get_status") {
        return handleGetStatus(args);
    }
    else if (name == "nst_deploy_project") {
        return handleDeployProject(args);
    }
    else if (name == "nst_save_workspace") {
        return handleSaveWorkspace(args);
    }

    // Unknown tool
    QJsonObject result;
    QJsonArray content;
    QJsonObject textObj;
    textObj["type"] = "text";
    textObj["text"] = QString("Unknown tool: %1").arg(name);
    content.append(textObj);
    result["content"] = content;
    result["isError"] = true;
    return result;
}

// ─── Tool Implementations ───────────────────────────────────────

QJsonObject McpServer::handleLoadProject(const QJsonObject &args)
{
    QString engine = args["engine"].toString();
    QString path = args["path"].toString();

    QJsonObject result;
    QJsonArray content;
    QJsonObject textObj;
    textObj["type"] = "text";

    if (m_core->loadProject(engine, path)) {
        int total = 0;
        auto data = m_core->projectDataManager()->getLoadedGameProjectData();
        for (auto it = data.begin(); it != data.end(); ++it) {
            total += it.value().size();
        }
        textObj["text"] = QString("Project loaded successfully. Engine: %1, Path: %2, Extracted: %3 strings.").arg(engine, path, QString::number(total));
        result["isError"] = false;
    } else {
        textObj["text"] = QString("Failed to load project: %1").arg(path);
        result["isError"] = true;
    }

    content.append(textObj);
    result["content"] = content;
    return result;
}

QJsonObject McpServer::handleStartTranslation(const QJsonObject &args)
{
    QString serviceName = args["serviceName"].toString();

    QJsonObject result;
    QJsonArray content;
    QJsonObject textObj;
    textObj["type"] = "text";

    // Trigger translateAll
    m_core->translateAll(serviceName);

    textObj["text"] = QString("Translation started. Monitor progress using 'nst_get_status'.");
    result["isError"] = false;

    content.append(textObj);
    result["content"] = content;
    return result;
}

QJsonObject McpServer::handleGetStatus(const QJsonObject &args)
{
    Q_UNUSED(args)

    QJsonObject result;
    QJsonObject statusObj;

    // Get current statistics from ProjectDataManager
    QString path = m_core->projectDataManager()->getProjectPath();
    QString engine = m_core->projectDataManager()->getEngineName();

    int totalTexts = 0;
    int translatedTexts = 0;

    auto data = m_core->projectDataManager()->getLoadedGameProjectData();
    for (auto it = data.begin(); it != data.end(); ++it) {
        const QJsonArray &entries = it.value();
        totalTexts += entries.size();
        for (const QJsonValue &val : entries) {
            if (!val.toObject()["text"].toString().isEmpty()) {
                translatedTexts++;
            }
        }
    }

    statusObj["projectPath"] = path;
    statusObj["engineName"] = engine;
    statusObj["totalTexts"] = totalTexts;
    statusObj["translatedTexts"] = translatedTexts;

    // Check if translating
    bool isTranslating = false;
    if (!m_logs.isEmpty()) {
        QString lastLog = m_logs.last();
        if (lastLog.contains("Translation pipeline started") || lastLog.contains("Progress updated")) {
            if (!lastLog.contains("Translation pipeline finished")) {
                isTranslating = true;
            }
        }
    }
    statusObj["isTranslating"] = isTranslating;

    double progress = totalTexts > 0 ? ((double)translatedTexts / totalTexts) * 100.0 : 0.0;
    statusObj["progressPercentage"] = progress;

    QJsonArray logsArray;
    // Return last 20 log messages
    int startIdx = qMax(0, m_logs.size() - 20);
    for (int i = startIdx; i < m_logs.size(); ++i) {
        logsArray.append(m_logs[i]);
    }
    statusObj["recentLogs"] = logsArray;

    QJsonArray content;
    QJsonObject textObj;
    textObj["type"] = "text";

    QJsonDocument doc(statusObj);
    textObj["text"] = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
    content.append(textObj);

    result["content"] = content;
    result["isError"] = false;
    return result;
}

QJsonObject McpServer::handleDeployProject(const QJsonObject &args)
{
    QString targetDir = args["targetDir"].toString();
    bool createBackup = args["createBackup"].toVariant().toBool();
    if (args["createBackup"].isUndefined()) {
        createBackup = true;
    }

    QJsonObject result;
    QJsonArray content;
    QJsonObject textObj;
    textObj["type"] = "text";

    if (m_core->deployProject(targetDir, createBackup)) {
        textObj["text"] = QString("Deployment successful. Files written to target directory.");
        result["isError"] = false;
    } else {
        textObj["text"] = QString("Deployment failed.");
        result["isError"] = true;
    }

    content.append(textObj);
    result["content"] = content;
    return result;
}

QJsonObject McpServer::handleSaveWorkspace(const QJsonObject &args)
{
    QString filePath = args["filePath"].toString();

    QJsonObject result;
    QJsonArray content;
    QJsonObject textObj;
    textObj["type"] = "text";

    if (m_core->saveWorkspace(filePath)) {
        textObj["text"] = QString("Workspace saved successfully to: %1").arg(filePath);
        result["isError"] = false;
    } else {
        textObj["text"] = QString("Failed to save workspace to: %1").arg(filePath);
        result["isError"] = true;
    }

    content.append(textObj);
    result["content"] = content;
    return result;
}

// ─── Response Helpers ───────────────────────────────────────────

QJsonObject McpServer::makeResponse(int id, const QJsonObject &result)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = id;
    message["result"] = result;
    return message;
}

QJsonObject McpServer::makeError(int id, int code, const QString &messageText, const QJsonObject &data)
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
    return message;
}
