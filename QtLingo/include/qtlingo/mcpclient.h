#ifndef QTLINGO_MCPCLIENT_H
#define QTLINGO_MCPCLIENT_H

#include "QtLingo_global.h"

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class QProcess;

namespace qtlingo {

struct QTLINGO_EXPORT McpServerConfig {
    QString name;
    QString command;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment;
    bool enabled = true;

    static McpServerConfig fromVariantMap(const QVariantMap &settings);
    QVariantMap toVariantMap() const;
};

struct QTLINGO_EXPORT McpTool {
    QString serverName;
    QString name;
    QString title;
    QString description;
    QJsonObject inputSchema;
};

class QTLINGO_EXPORT McpClient : public QObject {
    Q_OBJECT
public:
    explicit McpClient(QObject *parent = nullptr);
    ~McpClient() override;

    bool start(const McpServerConfig &config);
    void stop(int timeoutMs = 3000);

    bool isRunning() const;
    bool isInitialized() const;
    McpServerConfig config() const;

    int listTools(const QString &cursor = QString());
    int callTool(const QString &toolName, const QJsonObject &arguments = QJsonObject());
    int sendRequest(const QString &method, const QJsonObject &params = QJsonObject());
    void sendResponse(int id, const QJsonObject &result = QJsonObject());
    void sendError(int id, int code, const QString &message, const QJsonObject &data = QJsonObject());
    void sendNotification(const QString &method, const QJsonObject &params = QJsonObject());

signals:
    void initialized(const QJsonObject &serverInfo, const QJsonObject &capabilities);
    void toolsListed(const QList<qtlingo::McpTool> &tools, const QString &nextCursor);
    void toolCallFinished(const QString &toolName, const QJsonObject &result);
    void requestFinished(int id, const QString &method, const QJsonObject &result);
    void requestFailed(int id, const QString &method, const QString &message, const QJsonObject &error);
    void protocolRequest(int id, const QString &method, const QJsonObject &params);
    void protocolNotification(const QString &method, const QJsonObject &params);
    void serverLog(const QString &message);
    void disconnected(int exitCode, const QString &message);

private slots:
    void onStarted();
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessError();
    void onFinished(int exitCode);

private:
    struct PendingRequest {
        QString method;
        QString toolName;
    };

    void writeMessage(const QJsonObject &message);
    void processLine(const QByteArray &line);
    void handleResponse(const QJsonObject &message);
    void handleNotification(const QJsonObject &message);
    QList<McpTool> parseTools(const QJsonObject &result) const;

    QProcess *m_process;
    McpServerConfig m_config;
    QByteArray m_stdoutBuffer;
    QHash<int, PendingRequest> m_pendingRequests;
    int m_nextRequestId = 1;
    bool m_initialized = false;
};

} // namespace qtlingo

Q_DECLARE_METATYPE(qtlingo::McpTool)
Q_DECLARE_METATYPE(QList<qtlingo::McpTool>)

#endif // QTLINGO_MCPCLIENT_H
