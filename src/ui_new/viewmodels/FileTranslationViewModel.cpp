#include "FileTranslationViewModel.h"

#include "projectdatamanager.h"
#include "smartfiltermanager.h"
#include "bgadatamanager.h"
#include "translationservicemanager.h"

namespace nst::ui {

FileTranslationViewModel::FileTranslationViewModel(TranslationServiceManager *translationService,
                                                     QObject *parent)
    : BaseViewModel(parent)
    , m_translationService(translationService)
{
    initializeManagers();
}

FileTranslationViewModel::~FileTranslationViewModel() = default;

void FileTranslationViewModel::initializeManagers()
{
    // Create models
    m_fileListModel = new QStandardItemModel(this);
    m_translationModel = new QStandardItemModel(this);
    
    // Initialize managers
    m_projectDataManager = new ProjectDataManager(m_fileListModel, m_translationModel, this);
    m_smartFilterManager = new SmartFilterManager(this);
    m_bgaDataManager = new BGADataManager(this);
    
    // Connect signals
    connect(m_projectDataManager, &ProjectDataManager::processingFinished,
            this, &FileTranslationViewModel::onProcessingFinished);
}

double FileTranslationViewModel::progress() const
{
    if (m_totalEntries == 0) return 0.0;
    return static_cast<double>(m_translatedEntries) / m_totalEntries * 100.0;
}

void FileTranslationViewModel::openProject(const QString &engineName, const QString &projectPath)
{
    setBusy(true);
    setStatusMessage("Loading project...");
    
    m_projectPath = projectPath;
    m_engineName = engineName;
    
    m_projectDataManager->setProjectPath(projectPath);
    m_projectDataManager->setEngineName(engineName);
    
    // TODO: Integrate with BGA loader
    // m_bgaDataManager->loadProject(engineName, projectPath);
    
    emit projectChanged();
}

void FileTranslationViewModel::saveProject()
{
    if (!hasProject()) return;
    
    setBusy(true);
    setStatusMessage("Saving project...");
    
    m_projectDataManager->saveGameProject();
    
    setBusy(false);
    setStatusMessage("Project saved.");
}

void FileTranslationViewModel::deployProject(const QString &targetDir)
{
    if (!hasProject()) return;
    
    setBusy(true);
    setStatusMessage("Deploying project...");
    
    m_projectDataManager->exportGameProject(targetDir);
    
    setBusy(false);
    setStatusMessage("Project deployed.");
}

void FileTranslationViewModel::loadWorkspace(const QString &filePath)
{
    setBusy(true);
    setStatusMessage("Loading workspace...");
    
    if (m_projectDataManager->loadTranslationWorkspace(filePath)) {
        setStatusMessage("Workspace loaded.");
    } else {
        reportError("Failed to load workspace.");
    }
    
    setBusy(false);
    emit projectChanged();
}

void FileTranslationViewModel::saveWorkspace(const QString &filePath)
{
    setBusy(true);
    setStatusMessage("Saving workspace...");
    
    if (m_projectDataManager->saveTranslationWorkspace(filePath)) {
        setStatusMessage("Workspace saved.");
    } else {
        reportError("Failed to save workspace.");
    }
    
    setBusy(false);
}

void FileTranslationViewModel::selectFile(const QModelIndex &index)
{
    m_projectDataManager->onFileSelected(index);
}

void FileTranslationViewModel::translateSelected()
{
    // Get selected items from translation model
    // Start translation via translation service
    setBusy(true);
    setStatusMessage("Translating...");
    
    // TODO: Implement batch translation
}

void FileTranslationViewModel::translateAll()
{
    setBusy(true);
    setStatusMessage("Translating all entries...");
    
    // TODO: Implement translate all
}

void FileTranslationViewModel::updateTranslation(const QString &source, const QString &translation)
{
    m_projectDataManager->updateTranslation(source, translation);
    updateStats();
    emit translationUpdated(source, translation);
}

void FileTranslationViewModel::setSettings(const QString &apiKey, const QString &targetLang, bool googleApi,
                                            const QString &llmProvider, const QString &llmApiKey,
                                            const QString &llmModel, const QString &llmBaseUrl)
{
    m_apiKey = apiKey;
    m_targetLanguage = targetLang;
    m_useGoogleApi = googleApi;
    m_llmProvider = llmProvider;
    m_llmApiKey = llmApiKey;
    m_llmModel = llmModel;
    m_llmBaseUrl = llmBaseUrl;
}

void FileTranslationViewModel::onLoadingFinished(const QJsonArray &/*data*/)
{
    setBusy(false);
    setStatusMessage("Project loaded.");
    updateStats();
    emit projectChanged();
}

void FileTranslationViewModel::onTranslationFinished(const QString &source, const QString &translation)
{
    updateTranslation(source, translation);
}

void FileTranslationViewModel::onProcessingFinished()
{
    setBusy(false);
    updateStats();
    setStatusMessage("Ready.");
}

void FileTranslationViewModel::updateStats()
{
    m_totalEntries = m_translationModel->rowCount();
    
    // Count translated entries
    int translated = 0;
    for (int i = 0; i < m_translationModel->rowCount(); ++i) {
        QModelIndex translationCol = m_translationModel->index(i, 1); // Assuming column 1 is translation
        if (!translationCol.data().toString().isEmpty()) {
            translated++;
        }
    }
    m_translatedEntries = translated;
    
    emit statsChanged();
}

} // namespace nst::ui
