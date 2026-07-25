#include "luatranslationservice.h"
#include <QFileInfo>
#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonArray>
#include <QSettings>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

#include <QDir>

static void writeLogToFile(const QString &msg) {
    QString homeLogPath = QDir::homePath() + "/lua_plugin_debug.log";
    QFile homeFile(homeLogPath);
    if (homeFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&homeFile);
        stream << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz") << "] " << msg << "\n";
    }

    QFile localFile("lua_plugin_debug.log");
    if (localFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&localFile);
        stream << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz") << "] " << msg << "\n";
    }
}

// --- LuaWorker Implementation ---

LuaWorker::LuaWorker(const QString &scriptPath, QObject *parent)
    : QObject(parent)
    , m_scriptPath(scriptPath)
{
    initLua();
}

LuaWorker::~LuaWorker()
{
    if (L) {
        lua_close(L);
    }
}

bool LuaWorker::initLua()
{
    L = luaL_newstate();
    luaL_openlibs(L);
    // Set script name global
    QFileInfo fi(m_scriptPath);
    lua_pushstring(L, fi.fileName().toUtf8().constData());
    lua_setglobal(L, "__script_name");

    // Register HTTP function
    lua_register(L, "nst_http_request", lua_http_request);

    // Register Sleep function
    lua_register(L, "nst_sleep", [](lua_State* L) -> int {
        int ms = luaL_checkinteger(L, 1);
        QThread::msleep(ms);
        return 0;
    });
    
    // Register JSON functions
    lua_register(L, "nst_json_encode", [](lua_State* L) -> int {
        return LuaWorker::lua_json_encode(L);
    });

    lua_register(L, "nst_json_decode", [](lua_State* L) -> int {
        return LuaWorker::lua_json_decode(L);
    });

    // Register setting function
    lua_register(L, "nst_get_setting", [](lua_State* L) -> int {
        return LuaWorker::lua_get_setting(L);
    });

    // Register logging function
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, LuaWorker::lua_log, 1);
    lua_setglobal(L, "nst_log");

    if (luaL_dofile(L, m_scriptPath.toStdString().c_str()) != LUA_OK) {
        QString error = lua_tostring(L, -1);
        qCritical() << "Failed to load Lua script:" << m_scriptPath << error;
        writeLogToFile(QString("[INIT ERROR] Failed to load script %1: %2").arg(m_scriptPath, error));
        emit errorOccurred(QString("Failed to load script: %1").arg(error));
        lua_close(L);
        L = nullptr;
        return false;
    }
    writeLogToFile(QString("[INIT SUCCESS] Loaded script: %1").arg(m_scriptPath));
    return true;
}

int LuaWorker::lua_log(lua_State *L)
{
    // Get 'this' from upvalue
    LuaWorker* worker = static_cast<LuaWorker*>(lua_touserdata(L, lua_upvalueindex(1)));
    const char* msg = lua_tostring(L, 1);
    qDebug() << "[Lua]" << msg;
    writeLogToFile(QString("[LOG] %1").arg(msg ? msg : ""));
    if (worker) {
        emit worker->logMessage(QString::fromUtf8(msg));
    }
    return 0;
}

int LuaWorker::lua_http_request(lua_State *L)
{
    // Arguments: url, method, headers (table), body
    const char* urlStr = luaL_checkstring(L, 1);
    const char* method = luaL_optstring(L, 2, "GET");
    
    QNetworkRequest request(QUrl(QString::fromUtf8(urlStr)));
    
    // Process headers
    if (lua_istable(L, 3)) {
        lua_pushnil(L);
        while (lua_next(L, 3) != 0) {
            const char* key = lua_tostring(L, -2);
            const char* value = lua_tostring(L, -1);
            request.setRawHeader(QByteArray(key), QByteArray(value));
            lua_pop(L, 1);
        }
    }
    
    QByteArray body;
    if (lua_isstring(L, 4)) {
        body = QByteArray(lua_tostring(L, 4));
    }

    writeLogToFile(QString("[HTTP REQUEST] %1 %2 | Body len: %3").arg(method, urlStr).arg(body.size()));

    // Perform request synchronously (blocking this worker thread)
    QNetworkAccessManager manager;
    QNetworkReply *reply = nullptr;
    
    if (QString::compare(method, "POST", Qt::CaseInsensitive) == 0) {
        reply = manager.post(request, body);
    } else {
        reply = manager.get(request);
    }

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // Return result: body, status_code, headers
    QByteArray responseBody = reply->readAll();
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    writeLogToFile(QString("[HTTP RESPONSE] Status: %1 | Reply len: %2").arg(statusCode).arg(responseBody.size()));

    lua_pushlstring(L, responseBody.constData(), responseBody.size());
    lua_pushinteger(L, statusCode);

    // Return headers table as 3rd result
    lua_newtable(L);
    QList<QByteArray> headerList = reply->rawHeaderList();
    for (const QByteArray &head : headerList) {
        lua_pushstring(L, head.constData()); // key
        lua_pushstring(L, reply->rawHeader(head).constData()); // value
        lua_settable(L, -3);
    }
    
    reply->deleteLater();
    return 3;
}

// --- JSON Helpers ---

int LuaWorker::lua_json_encode(lua_State *L)
{
    if (lua_gettop(L) < 1) return 0;
    QJsonValue val = lua_to_json(L, 1);
    QJsonDocument doc;
    if (val.isObject()) doc.setObject(val.toObject());
    else if (val.isArray()) doc.setArray(val.toArray());
    else {
        lua_pushstring(L, ""); 
        return 1;
    }
    
    lua_pushstring(L, doc.toJson(QJsonDocument::Compact).constData());
    return 1;
}

int LuaWorker::lua_json_decode(lua_State *L)
{
    const char* jsonStr = luaL_checkstring(L, 1);
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(jsonStr));
    
    if (doc.isArray()) {
        json_to_lua(L, doc.array());
    } else if (doc.isObject()) {
        json_to_lua(L, doc.object());
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int LuaWorker::lua_get_setting(lua_State *L)
{
    const char* key = luaL_checkstring(L, 1);
    
    lua_getglobal(L, "__script_name");
    QString currentScriptName = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);
    
    if (currentScriptName.isEmpty()) {
         currentScriptName = "UnknownScript";
    }

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
    settings.beginGroup("Plugins");
    settings.beginGroup(currentScriptName);
    settings.beginGroup("Settings");
    
    QVariant val = settings.value(key);
    settings.endGroup();
    settings.endGroup();
    settings.endGroup();

    if (!val.isValid() || val.toString().trimmed().isEmpty()) {
        QSettings nativeSettings("NST", "PluginSettings");
        val = nativeSettings.value("Plugins/" + currentScriptName + "/Settings/" + key);
    }

    if (!val.isValid() || val.toString().trimmed().isEmpty()) {
        QSettings globalSettings;
        if (key == QString("api_key")) {
            val = globalSettings.value("llmApiKey", "");
        } else if (key == QString("base_url")) {
            val = globalSettings.value("llmBaseUrl", "");
        } else if (key == QString("model")) {
            val = globalSettings.value("llmModel", "");
        }
    }
    
    writeLogToFile(QString("[GET SETTING] Script: %1 | Key: %2 | Found: %3").arg(currentScriptName, key, val.isValid() && !val.toString().isEmpty() ? "YES" : "NO"));

    if (val.isValid() && !val.toString().isEmpty()) {
        lua_pushstring(L, val.toString().toUtf8().constData());
    } else {
        lua_pushnil(L);
    }
    return 1;
}

QJsonValue LuaWorker::lua_to_json(lua_State *L, int index)
{
    int type = lua_type(L, index);
    switch (type) {
        case LUA_TBOOLEAN: return QJsonValue((bool)lua_toboolean(L, index));
        case LUA_TNUMBER: return QJsonValue(lua_tonumber(L, index));
        case LUA_TSTRING: return QJsonValue(lua_tostring(L, index));
        case LUA_TTABLE: {
            // Check if array or object
            // Simple heuristic: if key 1 exists, assume array, else object
            lua_pushinteger(L, 1);
            lua_gettable(L, index);
            bool isArray = !lua_isnil(L, -1);
            lua_pop(L, 1);
            
            if (isArray) {
                QJsonArray arr;
                int len = lua_rawlen(L, index);
                for (int i = 1; i <= len; ++i) {
                    lua_rawgeti(L, index, i);
                    arr.append(lua_to_json(L, lua_gettop(L)));
                    lua_pop(L, 1);
                }
                return arr;
            } else {
                QJsonObject obj;
                lua_pushnil(L);
                while (lua_next(L, index) != 0) {
                    const char* key = lua_tostring(L, -2);
                    if (key) {
                        obj.insert(key, lua_to_json(L, lua_gettop(L)));
                    }
                    lua_pop(L, 1);
                }
                return obj;
            }
        }
        default: return QJsonValue();
    }
}

void LuaWorker::json_to_lua(lua_State *L, const QJsonValue &val)
{
    if (val.isBool()) lua_pushboolean(L, val.toBool());
    else if (val.isDouble()) lua_pushnumber(L, val.toDouble());
    else if (val.isString()) lua_pushstring(L, val.toString().toUtf8().constData());
    else if (val.isArray()) {
        lua_newtable(L);
        QJsonArray arr = val.toArray();
        for (int i = 0; i < arr.size(); ++i) {
            json_to_lua(L, arr[i]);
            lua_rawseti(L, -2, i + 1);
        }
    } else if (val.isObject()) {
        lua_newtable(L);
        QJsonObject obj = val.toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            lua_pushstring(L, it.key().toUtf8().constData());
            json_to_lua(L, it.value());
            lua_settable(L, -3);
        }
    } else {
        lua_pushnil(L);
    }
}

void LuaWorker::processTranslation(const QString &sourceText)
{
    if (!L) {
        if (!initLua()) return;
    }

    lua_getglobal(L, "on_text_extract");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        emit translationFinished({sourceText, sourceText});
        return;
    }

    lua_pushstring(L, sourceText.toUtf8().constData());

    // Expect 2 return values: result, error_message
    if (lua_pcall(L, 1, 2, 0) != LUA_OK) {
        QString error = lua_tostring(L, -1);
        qCritical() << "Error calling on_text_extract:" << error;
        emit errorOccurred(QString("Lua execution error: %1").arg(error));
        lua_pop(L, 1);
        return;
    }

    // Check first return value (translation)
    if (lua_isnil(L, -2)) {
        QString errorMsg = "Translation failed (Script returned nil)";
        // Check second return value (error message)
        if (lua_isstring(L, -1)) {
            errorMsg = QString::fromUtf8(lua_tostring(L, -1));
        }
        lua_pop(L, 2); // Pop both results
        emit errorOccurred(errorMsg);
        return;
    }

    QString result = sourceText;
    if (lua_isstring(L, -2)) {
        result = QString::fromUtf8(lua_tostring(L, -2));
    }
    lua_pop(L, 2); // Pop both results

    emit translationFinished({sourceText, result});
}

void LuaWorker::processBatchTranslation(const QStringList &sourceTexts)
{
    if (!L) {
        if (!initLua()) return;
    }

    writeLogToFile(QString("[BATCH TRANSLATION START] Processing batch of %1 items").arg(sourceTexts.size()));
    lua_getglobal(L, "on_batch_text_extract");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        qWarning() << "Script does not support on_batch_text_extract. Falling back to sequential.";
        writeLogToFile("[BATCH FALLBACK] on_batch_text_extract not found, using sequential on_text_extract");
        
        QList<qtlingo::TranslationResult> results;
        for (const QString &text : sourceTexts) {
             lua_getglobal(L, "on_text_extract");
             if (lua_isfunction(L, -1)) {
                 lua_pushstring(L, text.toUtf8().constData());
                 if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
                     QString res = text;
                     if (lua_isstring(L, -1)) res = QString::fromUtf8(lua_tostring(L, -1));
                     results.append({text, res});
                 } else {
                     results.append({text, text});
                 }
                 lua_pop(L, 1);
             } else {
                 lua_pop(L, 1);
                 results.append({text, text});
             }
        }
        writeLogToFile(QString("[BATCH FALLBACK DONE] Processed %1 items").arg(results.size()));
        emit batchTranslationFinished(results);
        return;
    }

    // Prepare table
    lua_newtable(L);
    for (int i = 0; i < sourceTexts.size(); ++i) {
        lua_pushstring(L, sourceTexts[i].toUtf8().constData());
        lua_rawseti(L, -2, i + 1);
    }

    // Call function
    // Expect 2 return values: result_table, error_message
    bool batchSuccess = false;
    QList<qtlingo::TranslationResult> results;

    if (lua_pcall(L, 1, 2, 0) == LUA_OK) {
        if (!lua_isnil(L, -2) && lua_istable(L, -2)) {
            int len = lua_rawlen(L, -2);
            for (int i = 1; i <= len; ++i) {
                lua_rawgeti(L, -2, i);
                QString res = "";
                if (lua_isstring(L, -1)) {
                    res = QString::fromUtf8(lua_tostring(L, -1));
                }
                lua_pop(L, 1);
                
                if (i - 1 < sourceTexts.size()) {
                    results.append({sourceTexts[i-1], res});
                }
            }
            batchSuccess = (results.size() == sourceTexts.size());
        }
        lua_pop(L, 2);
    } else {
        lua_pop(L, lua_gettop(L));
    }

    if (!batchSuccess) {
        writeLogToFile(QString("[BATCH FALLBACK] Batch translation failed or returned incomplete data. Falling back to sequential mode for %1 items...").arg(sourceTexts.size()));
        results.clear();
        for (const QString &text : sourceTexts) {
            lua_getglobal(L, "on_text_extract");
            if (lua_isfunction(L, -1)) {
                lua_pushstring(L, text.toUtf8().constData());
                if (lua_pcall(L, 1, 2, 0) == LUA_OK) {
                    QString res = text;
                    if (lua_isstring(L, -2)) {
                        res = QString::fromUtf8(lua_tostring(L, -2));
                    }
                    results.append({text, res});
                    lua_pop(L, 2);
                } else {
                    lua_pop(L, lua_gettop(L));
                    results.append({text, text});
                }
            } else {
                lua_pop(L, 1);
                results.append({text, text});
            }
        }
        writeLogToFile(QString("[BATCH FALLBACK SUCCESS] Processed %1 items sequentially").arg(results.size()));
    }

    writeLogToFile(QString("[BATCH SUCCESS] Finished %1 of %2 items").arg(results.size()).arg(sourceTexts.size()));
    emit batchTranslationFinished(results);
}

// --- LuaTranslationService Implementation ---

LuaTranslationService::LuaTranslationService(const QString &scriptPath, QObject *parent)
    : qtlingo::ITranslationService(parent)
{
    QFileInfo fi(scriptPath);
    m_serviceName = "Lua: " + fi.fileName();

    m_worker = new LuaWorker(scriptPath);
    m_worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &LuaTranslationService::startTranslation, m_worker, &LuaWorker::processTranslation);
    connect(this, &LuaTranslationService::startBatchTranslation, m_worker, &LuaWorker::processBatchTranslation);
    connect(m_worker, &LuaWorker::translationFinished, this, &LuaTranslationService::translationFinished);
    connect(m_worker, &LuaWorker::batchTranslationFinished, this, &LuaTranslationService::batchTranslationFinished);
    connect(m_worker, &LuaWorker::errorOccurred, this, &LuaTranslationService::errorOccurred);
    connect(m_worker, &LuaWorker::logMessage, this, &LuaTranslationService::logMessage); // Connect log signal

    m_workerThread.start();
}

LuaTranslationService::~LuaTranslationService()
{
    m_workerThread.quit();
    m_workerThread.wait();
}

QString LuaTranslationService::serviceName() const
{
    return m_serviceName;
}

void LuaTranslationService::translate(const QString &sourceText)
{
    emit startTranslation(sourceText);
}

void LuaTranslationService::batchTranslate(const QStringList &sourceTexts)
{
    emit startBatchTranslation(sourceTexts);
}
