#pragma once
#include <QObject>
#include <QMap>
#include <QMenu>
#include "LuaPlugin.h"

class LuaScriptManager : public QObject {
    Q_OBJECT
public:
    static LuaScriptManager& instance();
    
    void loadScriptsFromDir(const QString& dir);
    void registerAPI();
    QVariant executeHook(const QString& hookName, const QVariantList& args = {});
    QVariant executeHookForPlugin(const QString& pluginName, const QString& hookName, const QVariantList& args = {});
    bool hasHook(const QString& pluginName, const QString& hookName);
    void installAll();
    void addMenuItems(QMenu* menu);

signals:
    void logMessage(const QString& msg);
    void mockDataLoaded(const QJsonArray& data);

private:
    explicit LuaScriptManager(QObject *parent = nullptr) : QObject(parent) {}
    QMap<QString, LuaPlugin*> m_scripts;
    
    static int api_log(lua_State* L);
    static int api_translate(lua_State* L);
    static int api_install_package(lua_State* L);
    static int api_load_mock_data(lua_State* L);
};
