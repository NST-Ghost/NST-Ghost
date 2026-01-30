#ifndef NST_UI_FILETRANSLATIONVIEWMODEL_H
#define NST_UI_FILETRANSLATIONVIEWMODEL_H

#include "../viewmodels/BaseViewModel.h"
#include <QStandardItemModel>
#include <QJsonArray>

// Forward declarations from managers
class ProjectDataManager;
class SmartFilterManager;
class BGADataManager;
class TranslationServiceManager;

namespace nst::ui {

/**
 * @brief ViewModel for File Translation page.
 * 
 * Manages the state and business logic for file-based translation,
 * coordinating between ProjectDataManager, SmartFilterManager, and UI.
 */
class FileTranslationViewModel : public BaseViewModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasProject READ hasProject NOTIFY projectChanged)
    Q_PROPERTY(QString projectPath READ projectPath NOTIFY projectChanged)
    Q_PROPERTY(QString engineName READ engineName NOTIFY projectChanged)
    Q_PROPERTY(int totalEntries READ totalEntries NOTIFY statsChanged)
    Q_PROPERTY(int translatedEntries READ translatedEntries NOTIFY statsChanged)
    Q_PROPERTY(double progress READ progress NOTIFY statsChanged)

public:
    explicit FileTranslationViewModel(TranslationServiceManager *translationService,
                                       QObject *parent = nullptr);
    ~FileTranslationViewModel() override;

    // Properties
    bool hasProject() const { return !m_projectPath.isEmpty(); }
    QString projectPath() const { return m_projectPath; }
    QString engineName() const { return m_engineName; }
    int totalEntries() const { return m_totalEntries; }
    int translatedEntries() const { return m_translatedEntries; }
    double progress() const;

    // Models for views
    QStandardItemModel* fileListModel() const { return m_fileListModel; }
    QStandardItemModel* translationModel() const { return m_translationModel; }

    // Commands
    Q_INVOKABLE void openProject(const QString &engineName, const QString &projectPath);
    Q_INVOKABLE void saveProject();
    Q_INVOKABLE void deployProject(const QString &targetDir = QString());
    Q_INVOKABLE void loadWorkspace(const QString &filePath);
    Q_INVOKABLE void saveWorkspace(const QString &filePath);

    Q_INVOKABLE void selectFile(const QModelIndex &index);
    Q_INVOKABLE void translateSelected();
    Q_INVOKABLE void translateAll();
    Q_INVOKABLE void updateTranslation(const QString &source, const QString &translation);

    // Settings
    void setSettings(const QString &apiKey, const QString &targetLang, bool googleApi,
                     const QString &llmProvider, const QString &llmApiKey,
                     const QString &llmModel, const QString &llmBaseUrl);

signals:
    void projectChanged();
    void statsChanged();
    void fileSelected(const QString &filePath);
    void translationUpdated(const QString &source, const QString &translation);

private slots:
    void onLoadingFinished(const QJsonArray &data);
    void onTranslationFinished(const QString &source, const QString &translation);
    void onProcessingFinished();

private:
    void initializeManagers();
    void updateStats();

private:
    TranslationServiceManager *m_translationService = nullptr;
    ProjectDataManager *m_projectDataManager = nullptr;
    SmartFilterManager *m_smartFilterManager = nullptr;
    BGADataManager *m_bgaDataManager = nullptr;

    QStandardItemModel *m_fileListModel = nullptr;
    QStandardItemModel *m_translationModel = nullptr;

    QString m_projectPath;
    QString m_engineName;
    int m_totalEntries = 0;
    int m_translatedEntries = 0;

    // Settings
    QString m_apiKey;
    QString m_targetLanguage;
    bool m_useGoogleApi = false;
    QString m_llmProvider;
    QString m_llmApiKey;
    QString m_llmModel;
    QString m_llmBaseUrl;
};

} // namespace nst::ui

#endif // NST_UI_FILETRANSLATIONVIEWMODEL_H
