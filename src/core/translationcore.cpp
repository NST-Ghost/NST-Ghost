#ifndef TRANSLATIONCORE_CPP
#define TRANSLATIONCORE_CPP

#include "translationcore.h"
#include "rpgm_injection_exporter.h"
#include "rpgm_control_masker.h"
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

    // JSON interceptors
    connect(this, &TranslationCore::projectLoaded, this, [this](const QString &path) {
        if (m_jsonOutput) {
            std::cout << "{\"status\":\"loaded\",\"projectPath\":\"" << path.toStdString() << "\"}" << std::endl;
        }
    });
    connect(this, &TranslationCore::translationStarted, this, [this]() {
        if (m_jsonOutput) {
            std::cout << "{\"status\":\"started\"}" << std::endl;
        }
    });
    connect(this, &TranslationCore::fileProgressUpdated, this, [this](const QString &filePath, int processed, int total) {
        if (m_jsonOutput) {
            std::cout << "{\"status\":\"progress\",\"filePath\":\"" << filePath.toStdString() << "\",\"processed\":" << processed << ",\"total\":" << total << "}" << std::endl;
        }
    });
    connect(this, &TranslationCore::errorOccurred, this, [this](const QString &msg) {
        if (m_jsonOutput) {
            std::cout << "{\"status\":\"error\",\"message\":\"" << msg.toStdString() << "\"}" << std::endl;
        }
    });
    connect(this, &TranslationCore::translationFinished, this, [this]() {
        if (m_jsonOutput) {
            std::cout << "{\"status\":\"finished\"}" << std::endl;
        }
    });
}

TranslationCore::~TranslationCore()
{
    if (m_mcpClient) {
        m_mcpClient->stop();
    }
}

void TranslationCore::initializeManagers()
{
    std::cerr << "[Core] Debug: Creating TranslationServiceManager..." << std::endl;
    m_serviceManager = new TranslationServiceManager(this);
    std::cerr << "[Core] Debug: Creating BGADataManager..." << std::endl;
    m_bgaDataManager = new BGADataManager(this);
    std::cerr << "[Core] Debug: Creating SmartFilterManager..." << std::endl;
    m_smartFilterManager = new SmartFilterManager(this);
    std::cerr << "[Core] Debug: Creating ProjectDataManager..." << std::endl;
    m_projectDataManager = new ProjectDataManager(this); // No UI models
    
    std::cerr << "[Core] Debug: Creating McpClient..." << std::endl;
    m_mcpClient = new qtlingo::McpClient(this);
    std::cerr << "[Core] Debug: All managers created." << std::endl;

    connect(m_serviceManager, &TranslationServiceManager::translationFinished, 
            this, &TranslationCore::onTranslationFinished);
    
    connect(m_serviceManager, &TranslationServiceManager::errorOccurred, this, [this](const QString &msg) {
        emit errorOccurred(msg);
    });

    connect(m_serviceManager, &TranslationServiceManager::progressUpdated, this, [this](int current, int total) {
        emit fileProgressUpdated(m_currentFilePath, current, total);
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

bool TranslationCore::deployAsInjection(const QString &languageName)
{
    QString gamePath = m_projectDataManager->getProjectPath();
    if (gamePath.isEmpty()) {
        emit errorOccurred("No project loaded.");
        return false;
    }

    RpgmInjectionExporter exporter(this);
    connect(&exporter, &RpgmInjectionExporter::errorOccurred, this, &TranslationCore::errorOccurred);
    connect(&exporter, &RpgmInjectionExporter::progressUpdated, this, [this](int pct, const QString &msg) {
        emit totalProgressUpdated(pct, 100);
    });

    bool success = exporter.deploy(
        gamePath,
        m_projectDataManager->getLoadedGameProjectData(),
        true, // onlyTranslated
        languageName
    );

    return success;
}

void TranslationCore::setTranslationSettings(const TranslationSettings &settings)
{
    m_settings = settings;

    if (m_mcpClient) {
        if (settings.mcpEnabled) {
            qtlingo::McpServerConfig current = m_mcpClient->config();
            QStringList newArgs = settings.mcpServerArgs.split(' ', Qt::SkipEmptyParts);
            if (!m_mcpClient->isRunning() || 
                current.name != settings.mcpServerName || 
                current.command != settings.mcpServerCommand || 
                current.arguments != newArgs) 
            {
                std::cerr << "[Core] Debug: Restarting MCP Client: " << settings.mcpServerName.toStdString() << std::endl;
                m_mcpClient->stop();
                
                qtlingo::McpServerConfig config;
                config.name = settings.mcpServerName;
                config.command = settings.mcpServerCommand;
                config.arguments = newArgs;
                config.workingDirectory = QDir::currentPath();
                
                m_mcpClient->start(config);
            }
        } else {
            if (m_mcpClient->isRunning()) {
                std::cerr << "[Core] Debug: Stopping MCP Client..." << std::endl;
                m_mcpClient->stop();
            }
        }
    }
}

void TranslationCore::setJsonOutput(bool enabled)
{
    m_jsonOutput = enabled;
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
            QJsonObject obj = val.toObject();
            QString source = obj["source"].toString();
            QString translated = obj["text"].toString();
            if (source.isEmpty()) continue;
            if (!translated.isEmpty()) continue; // Skip already translated entries
            allSources.append(source);
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

    // RPG Maker Pre-Translation Control Code Restoration (Unmasking)
    if (RpgmControlMasker::isRpgmEngine(m_projectDataManager->getEngineName())) {
        if (m_rpgmTagMaps.contains(qr.result.sourceText)) {
            QMap<QString, QString> tagMap = m_rpgmTagMaps.value(qr.result.sourceText);
            qr.result.translatedText = RpgmControlMasker::unmask(qr.result.translatedText, tagMap);
            for (auto tagIt = tagMap.constBegin(); tagIt != tagMap.constEnd(); ++tagIt) {
                if (qr.result.sourceText.contains(tagIt.key())) {
                    qr.result.sourceText.replace(tagIt.key(), tagIt.value());
                }
            }
        } else if (qr.result.translatedText.contains("__NST_TAG_")) {
            for (auto mapIt = m_rpgmTagMaps.constBegin(); mapIt != m_rpgmTagMaps.constEnd(); ++mapIt) {
                qr.result.translatedText = RpgmControlMasker::unmask(qr.result.translatedText, mapIt.value());
            }
        }
    }
    
    // Find file path for this result
    if (m_pendingTranslations.contains(qr.result.sourceText)) {
        qr.filePath = m_pendingTranslations.value(qr.result.sourceText).filePath;
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
    m_currentFilePath = job.filePath;
    
    // RPG Maker Pre-Translation Control Code Masking
    if (RpgmControlMasker::isRpgmEngine(m_projectDataManager->getEngineName())) {
        QStringList maskedTexts;
        for (const QString &src : job.sourceTexts) {
            RpgmControlMasker::MaskResult res = RpgmControlMasker::mask(src);
            maskedTexts.append(res.maskedText);
            if (res.hasMaskedTags) {
                m_rpgmTagMaps[res.maskedText] = res.tagMap;
                m_rpgmTagMaps[src] = res.tagMap;
            }
        }
        job.sourceTexts = maskedTexts;
    }

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

    // Quick non-ASCII check (Kanji, Hiragana, Katakana, Thai, Cyrillic, Hangul)
    for (const QChar &c : text) {
        ushort u = c.unicode();
        if ((u >= 0x3000 && u <= 0x9FFF) || (u >= 0x0E00 && u <= 0x0E7F) || 
            (u >= 0x0400 && u <= 0x04FF) || (u >= 0xAC00 && u <= 0xD7AF)) {
            return false;
        }
    }

    static const QRegularExpression codePattern(
        R"(^(function\b|void\b|var\b|let\b|const\b|import\b|return\b|if\s*\(|while\s*\(|for\s*\()|\b(this\.|SceneManager\.|Graphics\.|AudioManager\.|renpy\.|config\.)|;\s*$)",
        QRegularExpression::CaseInsensitiveOption
    );
    if (codePattern.match(text.trimmed()).hasMatch()) return true;

    static const QRegularExpression pureSymbols("^[{}\\[\\]();,<>=\\+\\-\\*\\/\\%\\&\\|]+$");
    if (pureSymbols.match(text.trimmed()).hasMatch()) return true;

    return false;
}

#endif
