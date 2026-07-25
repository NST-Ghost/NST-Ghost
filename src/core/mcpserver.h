#ifndef MCPSERVER_H
#define MCPSERVER_H

#include "translationcore.h"
#include <QObject>
#include <QString>

namespace qtlingo {
    class McpLocalTransport;
}

/**
 * @brief MCP Server that exposes NST translation capabilities to AI agents.
 *
 * Supports two transport modes:
 * - **Stdio**: Traditional stdin/stdout transport for `--mcp-server` flag (separate process)
 * - **Local**: Embedded Unix Socket / Named Pipe transport (built into TUI/GUI process)
 *
 * In Local mode, the server uses McpLocalTransport and supports multiple concurrent clients.
 */
class McpServer : public QObject
{
    Q_OBJECT
public:
    enum TransportMode {
        Stdio,  ///< Read from stdin, write to stdout (legacy, --mcp-server)
        Local   ///< Listen on Unix socket / Named Pipe (embedded in TUI)
    };

    explicit McpServer(TranslationCore *core, QObject *parent = nullptr);
    ~McpServer();

    /// Start in stdio transport mode (reads stdin, writes stdout)
    void start();

    /// Start in local socket transport mode (embedded)
    bool startLocal(const QString &socketPath = QString());

    /// Stop the server (local mode only; stdio mode runs until EOF)
    void stop();

    /// Check if the embedded server is listening
    bool isListening() const;

    /// Get the socket path (local mode only)
    QString socketPath() const;

    /// Get the current transport mode
    TransportMode transportMode() const;

    /// Get the number of connected clients (local mode only)
    int connectedClientCount() const;

signals:
    /// Emitted when a client connects (local mode only)
    void clientConnected(int totalClients);

    /// Emitted when a client disconnects (local mode only)
    void clientDisconnected(int totalClients);

private:
    // Core request handler — transport-agnostic
    QJsonObject handleRequest(const QJsonObject &request);

    // Response helpers
    QJsonObject makeResponse(int id, const QJsonObject &result);
    QJsonObject makeError(int id, int code, const QString &message, const QJsonObject &data = QJsonObject());

    // Stdio transport helpers
    void handleStdioLine(const QString &line);
    void writeStdioMessage(const QJsonObject &message);

    // Local transport message handler
    void handleLocalMessage(quintptr clientId, const QJsonObject &message);

    // Tool Handlers
    QJsonObject handleInitialize(const QJsonObject &request);
    QJsonObject handleToolsList();
    QJsonObject handleToolCall(const QJsonObject &params);
    QJsonObject handleLoadProject(const QJsonObject &args);
    QJsonObject handleStartTranslation(const QJsonObject &args);
    QJsonObject handleGetStatus(const QJsonObject &args);
    QJsonObject handleDeployProject(const QJsonObject &args);
    QJsonObject handleSaveWorkspace(const QJsonObject &args);

    TranslationCore *m_core;
    QStringList m_logs;
    TransportMode m_transportMode = Stdio;
    qtlingo::McpLocalTransport *m_localTransport = nullptr;
};

#endif // MCPSERVER_H
