#ifndef QTLINGO_MCPLOCALTRANSPORT_H
#define QTLINGO_MCPLOCALTRANSPORT_H

#include "QtLingo_global.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QJsonObject>

class QLocalServer;
class QLocalSocket;

namespace qtlingo {

/**
 * @brief Local socket transport for embedded MCP server.
 *
 * Uses QLocalServer to listen on a Unix Domain Socket (Linux) or
 * Named Pipe (Windows). AI agents connect via the socket to send
 * JSON-RPC MCP requests, eliminating the need for a separate
 * MCP server process.
 *
 * Each connected client gets its own read buffer for message framing.
 */
class QTLINGO_EXPORT McpLocalTransport : public QObject
{
    Q_OBJECT
public:
    explicit McpLocalTransport(QObject *parent = nullptr);
    ~McpLocalTransport() override;

    /**
     * @brief Start listening on the given socket path.
     * @param socketPath Path for Unix socket or Named Pipe name.
     *        If empty, uses default: $XDG_RUNTIME_DIR/nst-mcp.sock (Linux)
     *        or \\.\pipe\nst-mcp (Windows).
     * @return true if server started listening successfully.
     */
    bool start(const QString &socketPath = QString());

    /**
     * @brief Stop listening and disconnect all clients.
     */
    void stop();

    /**
     * @brief Check if the transport is actively listening.
     */
    bool isListening() const;

    /**
     * @brief Get the actual socket path being used.
     */
    QString socketPath() const;

    /**
     * @brief Get the number of currently connected clients.
     */
    int connectedClientCount() const;

    /**
     * @brief Send a JSON-RPC message to a specific client.
     * @param clientId The ID of the client connection.
     * @param message The JSON object to send.
     */
    void sendMessage(quintptr clientId, const QJsonObject &message);

    /**
     * @brief Broadcast a JSON-RPC message to all connected clients.
     * @param message The JSON object to send.
     */
    void broadcastMessage(const QJsonObject &message);

signals:
    /**
     * @brief Emitted when a new client connects.
     * @param clientId Unique identifier for the connection.
     */
    void clientConnected(quintptr clientId);

    /**
     * @brief Emitted when a client disconnects.
     * @param clientId Unique identifier for the connection.
     */
    void clientDisconnected(quintptr clientId);

    /**
     * @brief Emitted when a complete JSON-RPC message is received from a client.
     * @param clientId The client that sent the message.
     * @param message The parsed JSON-RPC message.
     */
    void messageReceived(quintptr clientId, const QJsonObject &message);

    /**
     * @brief Emitted when the server encounters an error.
     * @param errorString Description of the error.
     */
    void serverError(const QString &errorString);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    struct ClientConnection {
        QLocalSocket *socket = nullptr;
        QByteArray readBuffer;
    };

    static QString defaultSocketPath();

    QLocalServer *m_server;
    QHash<quintptr, ClientConnection> m_clients;
    QString m_socketPath;
};

} // namespace qtlingo

#endif // QTLINGO_MCPLOCALTRANSPORT_H
