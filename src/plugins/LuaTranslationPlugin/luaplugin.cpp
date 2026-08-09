#include "luaplugin.h"
#include "luatranslationservice.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <iostream>
#include <lua.hpp>
#include <QSettings>

LuaTranslationPlugin::LuaTranslationPlugin(QObject *parent)
    : QObject(parent)
{
    scanScripts();
}

#include <QDirIterator>

void LuaTranslationPlugin::scanScripts()
{
    m_scriptMap.clear();
    QDir scriptDir(QCoreApplication::applicationDirPath());
    
#if defined(Q_OS_WIN)
    if (scriptDir.dirName().toLower() == "debug" || scriptDir.dirName().toLower() == "release")
        scriptDir.cdUp();
#elif defined(Q_OS_MAC)
    if (scriptDir.dirName() == "MacOS") {
        scriptDir.cdUp();
        scriptDir.cdUp();
        scriptDir.cdUp();
    }
#endif
    
    if (!scriptDir.cd("scripts")) {
         if(!QDir::current().cd("scripts")) {
             return; 
         }
         scriptDir = QDir::current();
         scriptDir.cd("scripts");
    }

    std::cerr << "Scanning scripts in: " << scriptDir.absolutePath().toStdString() << std::endl;
    QDirIterator it(scriptDir.absolutePath(), {"*.lua"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        if (filePath.contains("/templates/") || filePath.contains("/template/")) {
            continue;
        }
        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();
        
        // Check if script has on_text_extract
        lua_State *L = luaL_newstate();
        luaL_openlibs(L); // Required for os.getenv and others
        
        // Register NST API functions so scripts can reference them
        // (even if they're only called inside functions, Lua may check during loading)
        lua_register(L, "nst_get_setting", [](lua_State* L) -> int {
            lua_pushnil(L); // Dummy implementation for scanning
            return 1;
        });
        lua_register(L, "nst_http_request", [](lua_State* L) -> int {
            lua_pushstring(L, "");
            lua_pushinteger(L, 0);
            return 2;
        });
        lua_register(L, "nst_json_encode", [](lua_State* L) -> int {
            lua_pushstring(L, "");
            return 1;
        });
        lua_register(L, "nst_json_decode", [](lua_State* L) -> int {
            lua_pushnil(L);
            return 1;
        });
        lua_register(L, "nst_log", [](lua_State* L) -> int {
            return 0;
        });
        
        if (luaL_dofile(L, filePath.toStdString().c_str()) == LUA_OK) {
            lua_getglobal(L, "on_text_extract");
            if (lua_isfunction(L, -1)) {
                
                // Local scripts in scripts directory are installed by default
                QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
                bool enabled = settings.value("Plugins/" + fileName + "/Enabled", true).toBool();
                bool installed = settings.value("Plugins/" + fileName + "/Installed", true).toBool();
                
                if (enabled && installed) {
                    std::cerr << "Found translation script (Enabled): " << fileName.toStdString() << std::endl;
                    QString serviceName = "Lua: " + fileName;
                    m_scriptMap.insert(serviceName, filePath);
                } else {
                    std::cerr << "Found translation script (Disabled/Not Installed): " << fileName.toStdString() << std::endl;
                }
            } else {
                std::cerr << "Skipping script (no on_text_extract): " << fileName.toStdString() << std::endl;
            }
        } else {
            std::cerr << "Failed to load script for check: " << fileName.toStdString() << " Error: " << lua_tostring(L, -1) << std::endl;
        }
        lua_close(L);
    }
}

QStringList LuaTranslationPlugin::keys() const
{
    // Re-scan to pick up new scripts? Maybe not for now, to be safe.
    // const_cast<LuaTranslationPlugin*>(this)->scanScripts(); 
    return m_scriptMap.keys();
}

qtlingo::ITranslationService* LuaTranslationPlugin::create(const QString &key, QObject *parent)
{
    if (m_scriptMap.contains(key)) {
        return new LuaTranslationService(m_scriptMap.value(key), parent);
    }
    return nullptr;
}
