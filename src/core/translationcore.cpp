#ifndef TRANSLATIONCORE_CPP
#define TRANSLATIONCORE_CPP

#include "translationcore.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <iostream>

TranslationCore::TranslationCore(QObject *parent)
    : QObject(parent)
    , m_serviceManager(nullptr)
    , m_bgaDataManager(nullptr)
    , m_smartFilterManager(nullptr)
    , m_projectDataManager(nullptr)
{
    initializeManagers();
    
    m_resultTimer = new QTimer(this);
    m_resultTimer->setInterval(100);
    connect(m_resultTimer, &QTimer::timeout, this, &TranslationCore::processIncomingResults);
}

TranslationCore::~TranslationCore()
{
}

void TranslationCore::initializeManagers()
{
    // Managers are independent of UI now
    std::cerr << "[Core] Debug: Creating TranslationServiceManager..." << std::endl;
    m_serviceManager = new TranslationServiceManager(this);
    std::cerr << "[Core] Debug: Creating BGADataManager..." << std::endl;
    m_bgaDataManager = new BGADataManager(this);
    std::cerr << "[Core] Debug: Creating SmartFilterManager..." << std::endl;
    m_smartFilterManager = new SmartFilterManager(this);
    std::cerr << "[Core] Debug: Creating ProjectDataManager..." << std::endl;
    m_projectDataManager = new ProjectDataManager(nullptr, nullptr, this); // No UI models
    std::cerr << "[Core] Debug: All managers created." << std::endl;

    connect(m_serviceManager, &TranslationServiceManager::translationFinished, 
            this, &TranslationCore::onTranslationFinished);
    
    connect(m_serviceManager, &TranslationServiceManager::errorOccurred, this, [this](const QString &msg) {
        emit errorOccurred(msg);
    });
}

bool TranslationCore::loadProject(const QString &engineName, const QString &projectPath)
{
    std::cerr << "[Core] Debug: loadProject started, engine=" << engineName.toStdString() << ", path=" << projectPath.toStdString() << std::endl;
    if (this->m_projectDataManager == nullptr) {
        std::cerr << "[Core] FATAL: m_projectDataManager is null!" << std::endl;
        return false;
    }
    std::cerr << "[Core] Debug: checking directory..." << std::endl;
    if (!QDir(projectPath).exists()) {
        std::cerr << "[Core] Debug: path not found" << std::endl;
        emit errorOccurred("Project path does not exist: " + projectPath);
        return false;
    }
    std::cerr << "[Core] Debug: path exists, clearing data..." << std::endl;
    m_projectDataManager->clearAllData();
    m_projectDataManager->setProjectPath(projectPath);
    m_projectDataManager->setEngineName(engineName);

    // Extract strings using BGA
    std::cerr << "[Core] Debug: Extraction starting..." << std::endl;
    QJsonArray extracted = m_bgaDataManager->loadStringsFromGameProject(engineName, projectPath);
    std::cerr << "[Core] Debug: Extraction finished, entries: " << extracted.size() << std::endl;
    
    if (extracted.isEmpty()) {
        std::cerr << "[Core] Warning: No strings extracted or extraction failed." << std::endl;
    }

    // Manually trigger the "loading finished" logic in ProjectDataManager
    std::cerr << "[Core] Debug: Triggering onLoadingFinished (sync)..." << std::endl;
    m_projectDataManager->onLoadingFinished(extracted, true);
    std::cerr << "[Core] Debug: onLoadingFinished completed." << std::endl;
    
    emit projectLoaded(projectPath);
    return true;
}

void TranslationCore::saveProject()
{
    m_projectDataManager->saveGameProject();
}

bool TranslationCore::saveWorkspace(const QString &filePath)
{
    return m_projectDataManager->saveTranslationWorkspace(filePath);
}

bool TranslationCore::deployProject(const QString &targetDir, bool createBackup)
{
    QString gamePath = m_projectDataManager->getProjectPath();
    QString outputPath = targetDir.isEmpty() ? gamePath : targetDir;
    
    if (createBackup) {
        m_bgaDataManager->createBackup(gamePath, m_projectDataManager->getLoadedGameProjectData());
    }

    bool success = m_bgaDataManager->exportStringsToGameProject(
        m_projectDataManager->getEngineName(),
        gamePath,
        outputPath,
        m_projectDataManager->getLoadedGameProjectData(),
        true // onlyTranslated
    );

    return success;
}

void TranslationCore::setTranslationSettings(const QVariantMap &settings)
{
    m_settings = settings;
}

void TranslationCore::translateAll(const QString &serviceName)
{
    QStringList allFiles = m_projectDataManager->getLoadedGameProjectData().keys();
    translateFiles(allFiles, serviceName);
}

void TranslationCore::translateFiles(const QStringList &filePaths, const QString &serviceName)
{
    QString actualService = serviceName;
    if (actualService.isEmpty()) {
        QStringList available = m_serviceManager->getAvailableServices();
        if (available.isEmpty()) {
            emit errorOccurred("No translation services available.");
            return;
        }
        actualService = available.first();
    }

    int queuedFiles = 0;
    for (const QString &filePath : filePaths) {
        if (!m_projectDataManager->getLoadedGameProjectData().contains(filePath)) continue;

        const QJsonArray &entries = m_projectDataManager->getLoadedGameProjectData()[filePath];
        QStringList sourceTexts;
        
        // Prepare text for filtering
        QStringList allSources;
        for (const QJsonValue &val : entries) {
            allSources.append(val.toObject()["source"].toString());
        }

        QList<bool> skipFlags = m_smartFilterManager->shouldSkipBatch(allSources);

        for (int i = 0; i < allSources.size(); ++i) {
            if (skipFlags.value(i, false)) continue;
            if (isLikelyCode(allSources[i])) continue;
            sourceTexts.append(allSources[i]);
        }

        if (!sourceTexts.isEmpty()) {
            TranslationJob job;
            job.serviceName = actualService;
            job.sourceTexts = sourceTexts;
            job.settings = m_settings;
            job.filePath = filePath;
            m_jobQueue.enqueue(job);
            queuedFiles++;
            
            // Track pending
            for (const QString &src : sourceTexts) {
                PendingTranslation pt;
                pt.filePath = filePath;
                m_pendingTranslations.insert(src, pt);
            }
        }
    }

    if (queuedFiles > 0) {
        emit translationStarted();
        if (!m_isTranslating) {
            processNextJob();
        }
    } else {
        emit translationFinished(); // Nothing to do
    }
}

void TranslationCore::onTranslationFinished(const qtlingo::TranslationResult &result)
{
    QueuedResult qr;
    qr.result = result;
    
    // Find file path for this result
    if (m_pendingTranslations.contains(result.sourceText)) {
        qr.filePath = m_pendingTranslations.value(result.sourceText).filePath;
    }

    m_incomingResults.enqueue(qr);
    if (!m_resultTimer->isActive()) {
        m_resultTimer->start();
    }
}

void TranslationCore::processNextJob()
{
    if (m_jobQueue.isEmpty()) {
        m_isTranslating = false;
        // Check if results are also done
        if (m_incomingResults.isEmpty()) {
            emit translationFinished();
        }
        return;
    }

    m_isTranslating = true;
    TranslationJob job = m_jobQueue.dequeue();
    
    // Call the service manager
    m_serviceManager->translate(job.serviceName, job.sourceTexts, job.settings);
    
    // In CLI mode, we should be careful about Rate Limits.
    // We add a significant delay (2 seconds) between batch requests to avoid GOAWAY errors.
    int delay = 2000; 
    
    // Continue with next job
    QTimer::singleShot(delay, this, &TranslationCore::processNextJob);
}

void TranslationCore::processIncomingResults()
{
    if (m_incomingResults.isEmpty()) {
        m_resultTimer->stop();
        if (!m_isTranslating && m_jobQueue.isEmpty()) {
            emit translationFinished();
        }
        return;
    }

    int processed = 0;
    while (!m_incomingResults.isEmpty() && processed < 50) {
        QueuedResult qr = m_incomingResults.dequeue();
        processed++;
        
        m_projectDataManager->updateTranslation(qr.result.sourceText, qr.result.translatedText, qr.filePath);
        m_pendingTranslations.remove(qr.result.sourceText);
    }
}

bool TranslationCore::isLikelyCode(const QString &text) const
{
    if (text.isEmpty()) return true;
    if (text.length() < 2) return false;
    
    static const QRegularExpression codeRegex("^[{}\\[\\]();,<>]+$|[a-zA-Z0-9_]+\\(.*\\)");
    if (codeRegex.match(text).hasMatch()) return true;
    
    return false;
}

#endif
