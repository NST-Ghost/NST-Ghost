#ifndef LUAPLUGIN_H
#define LUAPLUGIN_H

#include <QObject>
#include <qtlingo/translationplugininterface.h>
#include <QMap>

class LuaTranslationPlugin : public QObject, public qtlingo::TranslationPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qtlingo.TranslationPluginInterface")
    Q_INTERFACES(qtlingo::TranslationPluginInterface)

public:
    explicit LuaTranslationPlugin(QObject *parent = nullptr);
    QStringList keys() const override;
    qtlingo::ITranslationService* create(const QString &key, QObject *parent = nullptr) override;

private:
    void scanScripts();
    QMap<QString, QString> m_scriptMap; // Service Name -> File Path
};

#endif // LUAPLUGIN_H
