#ifndef TRANSLATIONCORE_H
#define TRANSLATIONCORE_H

#include <QObject>
#include <QQueue>
#include <QMultiMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QStringList>

#include "bgadatamanager.h"
#include "translationservicemanager.h"
#include "smartfiltermanager.h"
#include "projectdatamanager.h"
#include "rpgm_injection_exporter.h"
#include <qtlingo/translationsettings.h>
#include <qtlingo/mcpclient.h>

namespace qtlingo {
    struct TranslationResult;
}

class TranslationCore : public QObject
{
    Q_OBJECT
public:
    explicit TranslationCore(QObject *parent = nullptr);
    ~TranslationCore();

    // Managers Access
    TranslationServiceManager* serviceManager() const { return m_serviceManager; }
    BGADataManager* bgaDataManager() const { return m_bgaDataManager; }
    SmartFilterManager* smartFilterManager() const { return m_smartFilterManager; }
    ProjectDataManager* projectDataManager() const { return m_projectDataManager; }
    qtlingo::McpClient* mcpClient() const { return m_mcpClient; }

    // Project Actions
    bool loadProject(const QString &engineName, const QString &projectPath);
    void saveProject(); // Saves metadata directly into game data files (DANGEROUS if called blindly)
    bool saveWorkspace(const QString &filePath); // Saves to a single .nst file
    bool deployProject(const QString &targetDir = QString(), bool createBackup = true);
    bool deployAsInjection(const QString &languageName = QStringLiteral("Thai"));

    // Translation Actions
    void translateAll(const QString &serviceName = QString());
    void translateFiles(const QStringList &filePaths, const QString &serviceName = QString());
    
    // Settings
    void setTranslationSettings(const TranslationSettings &settings);
    TranslationSettings translationSettings() const { return m_settings; }
    
    // JSON Output option for CLI
    void setJsonOutput(bool enabled);
    bool jsonOutput() const { return m_jsonOutput; }

signals:
    void projectLoaded(const QString &projectPath);
    void translationStarted();
    void translationFinished();
    void fileProgressUpdated(const QString &filePath, int processed, int total);
    void totalProgressUpdated(int processed, int total);
    void errorOccurred(const QString &message);

private slots:
    void onTranslationFinished(const qtlingo::TranslationResult &result);
    void processNextJob();
    void processIncomingResults();

private:
    void initializeManagers();
    bool isLikelyCode(const QString &text) const;

    // Translation Job structure
    struct TranslationJob {
        QString serviceName;
        QStringList sourceTexts;
        TranslationSettings settings;
        QString filePath;
    };

    struct PendingTranslation {
        QString filePath;
        // In the core, we don't track UI indices, but we might track IDs if needed
    };

    struct QueuedResult {
        qtlingo::TranslationResult result;
        QString filePath;
    };

    // Managers
    TranslationServiceManager *m_serviceManager;
    BGADataManager *m_bgaDataManager;
    SmartFilterManager *m_smartFilterManager;
    ProjectDataManager *m_projectDataManager;
    qtlingo::McpClient *m_mcpClient;

    // State
    QQueue<TranslationJob> m_jobQueue;
    QQueue<QueuedResult> m_incomingResults;
    QMultiMap<QString, PendingTranslation> m_pendingTranslations;
    bool m_isTranslating = false;
    TranslationSettings m_settings;
    QString m_currentFilePath;
    bool m_jsonOutput = false;

    QTimer *m_resultTimer;
    QMap<QString, QMap<QString, QString>> m_rpgmTagMaps;
};

#endif // TRANSLATIONCORE_H
