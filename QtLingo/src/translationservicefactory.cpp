#include "qtlingo/translationservicefactory.h"
#include "qtlingo/translationplugininterface.h"
#include "google_translate_service.h"
#include "llm_translation_service.h"
#include <QCoreApplication>
#include <QDir>
#include <QPluginLoader>
#include <QMap>
#include <QMutex>

namespace qtlingo {

namespace {

// Singleton to manage loaded plugins
class PluginManager {
public:
    static PluginManager& instance() {
        static PluginManager instance;
        return instance;
    }

    void loadPlugins() {
        if (m_loaded) return;
        QMutexLocker locker(&m_mutex);
        if (m_loaded) return;

        QDir pluginsDir(QCoreApplication::applicationDirPath());
        
#if defined(Q_OS_WIN)
        if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
            pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
        if (pluginsDir.dirName() == "MacOS") {
            pluginsDir.cdUp();
            pluginsDir.cdUp();
            pluginsDir.cdUp();
        }
#endif
        if (!pluginsDir.cd("plugins")) {
            // Try looking in the current directory if "plugins" doesn't exist relative to app dir
             // This might happen during development or testing
             if(!QDir::current().cd("plugins")) {
                 return; 
             }
             pluginsDir = QDir::current();
             pluginsDir.cd("plugins");
        }

        const auto entryList = pluginsDir.entryList(QDir::Files);
        for (const QString &fileName : entryList) {
            QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
            QObject *plugin = loader.instance();
            if (plugin) {
                auto *iPlugin = qobject_cast<TranslationPluginInterface *>(plugin);
                if (iPlugin) {
                    m_plugins.append(iPlugin);
                    for (const QString &key : iPlugin->keys()) {
                        m_pluginMap.insert(key, iPlugin);
                    }
                } else {
                    loader.unload();
                }
            }
        }
        m_loaded = true;
    }

    QStringList availableServices() {
        loadPlugins();
        QStringList services = {"Google Translate", "LLM Translation"};
        services.append(m_pluginMap.keys());
        services.removeDuplicates();
        return services;
    }

    std::unique_ptr<ITranslationService> create(const QString &serviceName, QObject *parent) {
        loadPlugins();
        if (m_pluginMap.contains(serviceName)) {
            return std::unique_ptr<ITranslationService>(m_pluginMap.value(serviceName)->create(serviceName, parent));
        }
        return nullptr;
    }

private:
    PluginManager() = default;
    ~PluginManager() = default; // Plugins are owned by QPluginLoader, which unloads them on destruction usually, but here we keep them alive.
    
    bool m_loaded = false;
    QMutex m_mutex;
    QList<TranslationPluginInterface*> m_plugins;
    QMap<QString, TranslationPluginInterface*> m_pluginMap;
};

} // namespace

std::unique_ptr<ITranslationService> createTranslationService(const QString &serviceName, QObject *parent)
{
    if (serviceName.compare("Google Translate", Qt::CaseInsensitive) == 0) {
        return std::make_unique<GoogleTranslateService>(parent);
    }
    if (serviceName.compare("LLM Translation", Qt::CaseInsensitive) == 0) {
        return std::make_unique<LLMTranslationService>(parent);
    }
    
    // Try plugins
    return PluginManager::instance().create(serviceName, parent);
}

QStringList availableTranslationServices()
{
    return PluginManager::instance().availableServices();
}

} // namespace qtlingo

