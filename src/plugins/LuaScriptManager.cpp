#include "LuaScriptManager.h"
#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QAction>
#include <QJsonArray>
#include <QJsonObject>

LuaScriptManager& LuaScriptManager::instance() {
    static LuaScriptManager inst;
    return inst;
}

#include <QDirIterator>

void LuaScriptManager::loadScriptsFromDir(const QString& dir) {
    QDirIterator it(dir, {"*.lua"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);
        QString file = fileInfo.fileName();
        auto* plugin = new LuaPlugin(this);
        if (plugin->loadScript(filePath)) {
            m_scripts[file] = plugin;
            qDebug() << "Loaded Lua script:" << file;
        } else {
            delete plugin;
        }
    }
}

void LuaScriptManager::registerAPI() {
    for (auto* plugin : m_scripts) {
        plugin->registerFunction("nst_log", api_log);
        plugin->registerFunction("nst_translate", api_translate);
        plugin->registerFunction("nst_install_package", api_install_package);
        plugin->registerFunction("nst_load_mock_data", api_load_mock_data);
    }
}

void LuaScriptManager::installAll() {
    for (auto* plugin : m_scripts) {
        plugin->callFunction("on_install");
    }
}

void LuaScriptManager::addMenuItems(QMenu* menu) {
    for (auto it = m_scripts.begin(); it != m_scripts.end(); ++it) {
        auto result = it.value()->callFunction("get_menu_items");
        if (result.isValid()) {
            auto* action = menu->addAction(result.toString());
            connect(action, &QAction::triggered, [plugin = it.value()]() {
                plugin->callFunction("on_menu_click");
            });
        }
    }
}

QVariant LuaScriptManager::executeHook(const QString& hookName, const QVariantList& args) {
    QVariant result;
    for (auto* plugin : m_scripts) {
        result = plugin->callFunction(hookName, args);
    }
    return result;
}

QVariant LuaScriptManager::executeHookForPlugin(const QString& pluginName, const QString& hookName, const QVariantList& args) {
    if (m_scripts.contains(pluginName)) {
        return m_scripts[pluginName]->callFunction(hookName, args);
    }
    return QVariant();
}

bool LuaScriptManager::hasHook(const QString& pluginName, const QString& hookName) {
    if (m_scripts.contains(pluginName)) {
        return m_scripts[pluginName]->hasFunction(hookName);
    }
    return false;
}

int LuaScriptManager::api_log(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    qDebug() << "[Lua]" << msg;
    emit instance().logMessage(QString::fromUtf8(msg));
    return 0;
}

int LuaScriptManager::api_translate(lua_State* L) {
    const char* text = lua_tostring(L, 1);
    lua_pushstring(L, text);
    return 1;
}

int LuaScriptManager::api_install_package(lua_State* L) {
    const char* cmd = lua_tostring(L, 1);
    QProcess process;
    process.start("sh", {"-c", cmd});
    process.waitForFinished(-1);
    lua_pushboolean(L, process.exitCode() == 0);
    return 1;
}

int LuaScriptManager::api_load_mock_data(lua_State* L) {
    // รับข้อมูลจาก Lua table
    QJsonArray data;
    
    if (lua_istable(L, 1)) {
        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            if (lua_istable(L, -1)) {
                QJsonObject item;
                
                lua_pushstring(L, "source");
                lua_gettable(L, -2);
                if (lua_isstring(L, -1)) {
                    item["source"] = QString::fromUtf8(lua_tostring(L, -1));
                }
                lua_pop(L, 1);
                
                lua_pushstring(L, "translation");
                lua_gettable(L, -2);
                if (lua_isstring(L, -1)) {
                    item["translation"] = QString::fromUtf8(lua_tostring(L, -1));
                }
                lua_pop(L, 1);
                
                data.append(item);
            }
            lua_pop(L, 1);
        }
    }
    
    emit instance().mockDataLoaded(data);
    return 0;
}
