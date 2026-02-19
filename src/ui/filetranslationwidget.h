#ifndef FILETRANSLATIONWIDGET_H
#define FILETRANSLATIONWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QTimer>
#include <QQueue>
#include <QMultiMap>
#include <QJsonObject>
#include <QJsonArray>

#include "customprogressdialog.h"
#include "searchcontroller.h"
#include "searchdialog.h"
#include "shortcutcontroller.h"
#include "bgadatamanager.h"
#include "translationservicemanager.h"
#include "smartfiltermanager.h"
#include "projectdatamanager.h"
// #include "qtlingo/TranslationResult.h" // Removed: Defined in translationservice.h

QT_BEGIN_NAMESPACE
namespace Ui { class FileTranslationWidget; }
QT_END_NAMESPACE

class FileTranslationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileTranslationWidget(TranslationServiceManager *serviceManager, QWidget *parent = nullptr);
    ~FileTranslationWidget();

    void openMockData();
    void setSettings(const QString &apiKey, const QString &targetLang, bool googleApi, 
                     const QString &llmProvider, const QString &llmApiKey, 
                     const QString &llmModel, const QString &llmBaseUrl,
                     const QString &sourceLanguage = "auto");
    
    // Accessor for ProjectDataManager
    ProjectDataManager* getProjectDataManager() const { return m_projectDataManager; }
    BGADataManager* getBGADataManager() const { return m_bgaDataManager; } // Added accessor

    // Actions triggered from Main Menu
    void onToggleContext(bool checked);
    void onHideCompleted(bool checked);
    void onExportSmartFilterRules();
    void onImportSmartFilterRules();
    // New Project Flow (cliMode=true skips save dialog and auto-saves)
    void onNewProject(const QString &engineName, const QString &projectPath, bool cliMode = false); 
    void onOpenProject();  // Renamed from onLoadTranslationWorkspace
    void onSaveProject();  // Renamed from onSaveGameProject
    void onDeployProject(); // Renamed from onExportGameProject
    void onDeployProjectCLI(const QString &targetDir = QString(), bool createBackup = true); // CLI deploy

    void onUndoTranslation();
    
    // AI Settings Access
    void setAiFilterEnabled(bool enabled);
    bool isAiFilterEnabled() const;
    void setAiFilterThreshold(double threshold);
    double aiFilterThreshold() const;

    // Use this path for deployment if set (avoids dialog)
    void setDefaultDeploymentPath(const QString &path);
    QString defaultDeploymentPath() const;

    void openFontManager(); // Added
    
signals:
    void projectLoaded(const QString &projectPath);

public slots:
    void openSearchDialog();
    void onSelectAllRequested();

private slots:
    void onLoadingFinished();
    void onProjectProcessingFinished();
    void onSearchResultSelected(const QString &fileName, int row);
    void onBGADataError(const QString &message);
    void onFontsLoaded(const QJsonArray &fonts); // Added
    void onSearchRequested(const QString &query);
    
    // Translation slots
    void onTranslationFinished(const qtlingo::TranslationResult &result);
    void onTranslationServiceError(const QString &message);
    void onTranslationTableViewCustomContextMenuRequested(const QPoint &pos);
    void onTranslateSelectedTextWithService();
    void onTranslateAllSelectedText();
    void onTranslateSelectedFiles(); // Note: This seemed to be missing implementation in original but declared
    
    void onTranslationDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onFileListCustomContextMenuRequested(const QPoint &pos);
    
    void onMarkAsIgnored();
    void onUnmarkAsIgnored();
    
    // AI Smart Filter
    void onAILearnRequested();
    void onAIUnlearnRequested();
    
    void processIncomingResults();

private:
    // Setup methods (constructor organization)
    void initializeModels();
    void initializeManagers();
    void setupTableView();
    void setupFileListView();
    void connectManagerSignals();
    void setupTimers();
    
    void processNextTranslationJob();
    bool isLikelyCode(const QString &text) const;

private:
    Ui::FileTranslationWidget *ui;
    TranslationServiceManager *m_translationServiceManager; // Owned by MainWindow
    
    QStandardItemModel *m_fileListModel;
    QStandardItemModel *m_translationModel;
    
    SearchController *m_searchController;
    SearchDialog *m_searchDialog;
    ShortcutController *m_shortcutController;
    BGADataManager *m_bgaDataManager;
    SmartFilterManager *m_smartFilterManager;
    ProjectDataManager *m_projectDataManager;
    
    CustomProgressDialog *m_progressDialog;
    QFutureWatcher<QJsonArray> m_loadFutureWatcher;
    
    // Settings (cached locally for use in translation jobs)
    QString m_apiKey;
    QString m_targetLanguage;
    QString m_engineName;
    QString m_currentProjectFile; // Track the current .nst file path
    bool m_googleApi;
    QString m_llmProvider;
    QString m_llmApiKey;
    QString m_llmModel;
    QString m_llmBaseUrl;
    QString m_sourceLanguage = "auto";
    
    QString m_defaultDeploymentPath; // Path set via CLI/Config to use as default
    
    // Queues and Timers
    struct PendingTranslation {
        QModelIndex index;
        QString filePath;
    };
    QMultiMap<QString, PendingTranslation> m_pendingTranslations;
    
    QVector<QModelIndex> m_pendingUIUpdates;
    QTimer *m_uiUpdateTimer;
    
    bool m_isImporting = false; // Flag to track import state
    QJsonArray m_gameFonts; // Added
    
    QTimer *m_spinnerTimer;
    int m_spinnerFrame = 0;
    QModelIndex m_currentTranslatingFileIndex;
    
    struct TranslationJob {
        QString serviceName;
        QStringList sourceTexts;
        QVariantMap settings;
        QModelIndex fileIndex;
    };
    QQueue<TranslationJob> m_translationQueue;
    bool m_isTranslating = false;
    
    struct QueuedTranslationResult {
        qtlingo::TranslationResult result;
        QString filePath;
    };
    QQueue<QueuedTranslationResult> m_incomingResults;
    QTimer *m_resultProcessingTimer;
};

#endif // FILETRANSLATIONWIDGET_H
