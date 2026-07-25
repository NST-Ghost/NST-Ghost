#ifndef LUATRANSLATIONSERVICE_H
#define LUATRANSLATIONSERVICE_H

#include <qtlingo/translationservice.h>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QNetworkAccessManager>
#include <lua.hpp>

// Worker class that runs in a separate thread
class LuaWorker : public QObject
{
    Q_OBJECT
public:
    explicit LuaWorker(const QString &scriptPath, QObject *parent = nullptr);
    ~LuaWorker();

public slots:
    void processTranslation(const QString &sourceText);
    void processBatchTranslation(const QStringList &sourceTexts);

signals:
    void translationFinished(const qtlingo::TranslationResult &result);
    void batchTranslationFinished(const QList<qtlingo::TranslationResult> &results);
    void errorOccurred(const QString &message);
    void logMessage(const QString &message); // New signal

private:
    QString m_scriptPath;
    lua_State *L = nullptr;
    
    bool initLua();
    static int lua_http_request(lua_State *L);
    static int lua_json_encode(lua_State *L);
    static int lua_json_decode(lua_State *L);
    static int lua_get_setting(lua_State *L);
    static int lua_log(lua_State *L); // Changed to static helper that delegates
    
    // Helpers for JSON conversion
    static QJsonValue lua_to_json(lua_State *L, int index);
    static void json_to_lua(lua_State *L, const QJsonValue &val);
};

class LuaTranslationService : public qtlingo::ITranslationService
{
    Q_OBJECT
public:
    explicit LuaTranslationService(const QString &scriptPath, QObject *parent = nullptr);
    ~LuaTranslationService();

    QString serviceName() const override;
    void translate(const QString &sourceText) override;
    
    bool supportsBatchTranslation() const override { return true; }
    void batchTranslate(const QStringList &sourceTexts) override;

signals:
    void startTranslation(const QString &sourceText);
    void startBatchTranslation(const QStringList &sourceTexts);
    void logMessage(const QString &message); // New signal

private:
    QString m_serviceName;
    QThread m_workerThread;
    LuaWorker *m_worker;
};

#endif // LUATRANSLATIONSERVICE_H
