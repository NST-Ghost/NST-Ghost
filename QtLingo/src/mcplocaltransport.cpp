#include "qtlingo/mcplocaltransport.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>

#include <iostream>

namespace qtlingo {

McpLocalTransport::McpLocalTransport(QObject *parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection,
            this, &McpLocalTransport::onNewConnection);
}

McpLocalTransport::~McpLocalTransport()
{
    stop();
}

QString McpLocalTransport::defaultSocketPath()
{
#ifdef _WIN32
    return QStringLiteral("nst-mcp");
#else
    // Prefer XDG_RUNTIME_DIR for per-user, secure socket placement
    QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtimeDir.isEmpty()) {
        runtimeDir = QDir::tempPath();
    }
    return runtimeDir + QStringLiteral("/nst-mcp.sock");
#endif
}

bool McpLocalTransport::start(const QString &socketPath)
{
    if (m_server->isListening()) {
        stop();
    }

    m_socketPath = socketPath.isEmpty() ? defaultSocketPath() : socketPath;

#ifndef _WIN32
    // On Unix, remove stale socket file if it exists from a previous crash
    if (QFile::exists(m_socketPath)) {
        // Try connecting to check if another instance is running
        QLocalSocket testSocket;
        testSocket.connectToServer(m_socketPath);
        if (testSocket.waitForConnected(500)) {
            // Another instance is running on this socket
            testSocket.disconnectFromServer();
            emit serverError(QStringLiteral("Another NST MCP server is already listening on: ") + m_socketPath);
            return false;
        }
        // Stale socket — remove it
        QFile::remove(m_socketPath);
    }
#endif

    if (!m_server->listen(m_socketPath)) {
        QString errorMsg = QStringLiteral("Failed to start MCP local transport on '%1': %2")
                               .arg(m_socketPath, m_server->errorString());
        emit serverError(errorMsg);
        std::cerr << "[MCP Transport] " << errorMsg.toStdString() << std::endl;
        return false;
    }

    std::cerr << "[MCP Transport] Listening on: " << m_socketPath.toStdString() << std::endl;
    return true;
}

void McpLocalTransport::stop()
{
    // Disconnect all clients
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it->socket) {
            it->socket->disconnectFromServer();
            it->socket->deleteLater();
        }
    }
    m_clients.clear();

    if (m_server->isListening()) {
        m_server->close();
    }

#ifndef _WIN32
    // Clean up socket file
    if (!m_socketPath.isEmpty() && QFile::exists(m_socketPath)) {
        QFile::remove(m_socketPath);
    }
#endif

    std::cerr << "[MCP Transport] Stopped." << std::endl;
}

bool McpLocalTransport::isListening() const
{
    return m_server->isListening();
}

QString McpLocalTransport::socketPath() const
{
    return m_socketPath;
}

int McpLocalTransport::connectedClientCount() const
{
    return m_clients.size();
}

void McpLocalTransport::sendMessage(quintptr clientId, const QJsonObject &message)
{
    auto it = m_clients.find(clientId);
    if (it == m_clients.end() || !it->socket) {
        return;
    }

    QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    payload.append('\n');
    it->socket->write(payload);
    it->socket->flush();
}

void McpLocalTransport::broadcastMessage(const QJsonObject &message)
{
    QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    payload.append('\n');

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it->socket && it->socket->isOpen()) {
            it->socket->write(payload);
            it->socket->flush();
        }
    }
}

void McpLocalTransport::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QLocalSocket *socket = m_server->nextPendingConnection();
        if (!socket) continue;

        quintptr clientId = reinterpret_cast<quintptr>(socket);

        ClientConnection conn;
        conn.socket = socket;
        m_clients.insert(clientId, conn);

        connect(socket, &QLocalSocket::readyRead,
                this, &McpLocalTransport::onClientReadyRead);
        connect(socket, &QLocalSocket::disconnected,
                this, &McpLocalTransport::onClientDisconnected);

        std::cerr << "[MCP Transport] Client connected (id=" << clientId
                  << ", total=" << m_clients.size() << ")" << std::endl;

        emit clientConnected(clientId);
    }
}

void McpLocalTransport::onClientReadyRead()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (!socket) return;

    quintptr clientId = reinterpret_cast<quintptr>(socket);
    auto it = m_clients.find(clientId);
    if (it == m_clients.end()) return;

    it->readBuffer.append(socket->readAll());

    // Process complete newline-delimited JSON messages
    qsizetype newlineIndex = it->readBuffer.indexOf('\n');
    while (newlineIndex != -1) {
        QByteArray line = it->readBuffer.left(newlineIndex).trimmed();
        it->readBuffer.remove(0, newlineIndex + 1);

        if (!line.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                emit messageReceived(clientId, doc.object());
            } else {
                std::cerr << "[MCP Transport] Parse error from client " << clientId
                          << ": " << parseError.errorString().toStdString() << std::endl;
            }
        }

        newlineIndex = it->readBuffer.indexOf('\n');
    }
}

void McpLocalTransport::onClientDisconnected()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (!socket) return;

    quintptr clientId = reinterpret_cast<quintptr>(socket);

    m_clients.remove(clientId);
    socket->deleteLater();

    std::cerr << "[MCP Transport] Client disconnected (id=" << clientId
              << ", remaining=" << m_clients.size() << ")" << std::endl;

    emit clientDisconnected(clientId);
}

} // namespace qtlingo
