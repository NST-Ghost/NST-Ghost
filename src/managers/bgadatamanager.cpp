#include "bgadatamanager.h"
#include <QThread> // Required for QThread::currentThreadId()
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QDateTime>
#include <iostream>

BGADataManager::BGADataManager(QObject *parent)
    : QObject(parent)
{
}

QJsonArray BGADataManager::loadedFonts() const
{
    return m_loadedFonts;
}

QStringList BGADataManager::getAvailableAnalyzers() const
{
    return core::availableAnalyzers();
}

QJsonArray BGADataManager::loadStringsFromGameProject(const QString &engineName, const QString &projectPath)
{
    qDebug() << "BGADataManager: loadStringsFromGameProject called in thread:" << QThread::currentThreadId();
    emit progressUpdated(0, "Starting project analysis...");

    QString logFilePath = "bgadatamanager_log.txt";
    QFile logFile(logFilePath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return QJsonArray();
    }
    QTextStream logStream(&logFile);

    QJsonArray extractedTextsArray;

    logStream << "BGADataManager: Creating analyzer for engine: " << engineName << "\n";
    std::unique_ptr<core::IGameAnalyzer> analyzer = core::createAnalyzer(engineName);
    if (!analyzer) {
        emit errorOccurred(QString("Failed to create analyzer for engine: %1").arg(engineName));
        logStream << "BGADataManager: Failed to create analyzer." << "\n";
        logFile.close();
        emit loadingFinished();
        return extractedTextsArray;
    }

    std::cerr << "[BGA] Analyzing game project at: " << projectPath.toStdString() << " using engine: " << engineName.toStdString() << std::endl;
    emit progressUpdated(25, "Analyzing game project...");
    logStream << "BGADataManager: Calling analyzer->analyze for project: " << projectPath << "\n";
    core::AnalyzerOutput output = analyzer->analyze(projectPath);
    logStream << "BGADataManager: Analyzer output payload size:" << output.payload.size() << "bytes" << "\n";

    if (!output.errorMessage.isEmpty()) { // Check for error message from analyzer
        emit errorOccurred(output.errorMessage);
        logStream << "BGADataManager: Analyzer returned error: " << output.errorMessage << "\n";
        logFile.close();
        emit loadingFinished();
        return extractedTextsArray; // Return empty array on error
    }

    if (output.payload.isEmpty()) {
        emit errorOccurred(QString("No data extracted from project: %1").arg(projectPath));
        logStream << "BGADataManager: No data extracted from project." << "\n";
        logFile.close();
        emit loadingFinished();
        return extractedTextsArray;
    }

    emit progressUpdated(75, "Parsing extracted data...");
    logStream << "BGADataManager: Before QJsonDocument::fromJson" << "\n";
    if (output.format == "application/json") {
        QJsonDocument doc = QJsonDocument::fromJson(output.payload);
        logStream << "BGADataManager: After QJsonDocument::fromJson" << "\n";
        if (doc.isNull()) {
            emit errorOccurred("Invalid JSON output from analyzer.");
            logStream << "BGADataManager: Invalid JSON output from analyzer." << "\n";
            logFile.close();
            emit loadingFinished();
            return extractedTextsArray;
        }

        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();
            if (rootObj.contains("strings") && rootObj["strings"].isArray()) {
                extractedTextsArray = rootObj["strings"].toArray();
            }
            if (rootObj.contains("fonts") && rootObj["fonts"].isArray()) {
                m_loadedFonts = rootObj["fonts"].toArray();
                emit fontsLoaded(m_loadedFonts);
            }
        } else if (doc.isArray()) {
            extractedTextsArray = doc.array();
        }

        emit progressUpdated(90, QString("Extracted %1 entries.").arg(extractedTextsArray.size()));
        logStream << "BGADataManager: Extracted " << extractedTextsArray.size() << " entries." << "\n";
        logStream << "BGADataManager: After extracting entries" << "\n";

    } else {
        logStream << "BGADataManager: Unsupported format detected: " << output.format << "\n";
        emit errorOccurred(QString("Unsupported analyzer output format: %1").arg(output.format));
        logStream << "BGADataManager: Unsupported analyzer output format." << "\n";
        logFile.close();
        emit loadingFinished();
        return extractedTextsArray;
    }
    logStream << "BGADataManager: loadStringsFromGameProject returning." << "\n";
    logFile.close();
    emit loadingFinished();
    qDebug() << "BGADataManager: loadStringsFromGameProject finished in thread:" << QThread::currentThreadId();
    return extractedTextsArray;
}
    
bool BGADataManager::saveStringsToGameProject(const QString &engineName, const QString &projectPath, const QMap<QString, QJsonArray> &data)
{
    std::unique_ptr<core::IGameAnalyzer> analyzer = core::createAnalyzer(engineName);
    if (!analyzer) {
        emit errorOccurred(QString("Failed to create analyzer for engine: %1").arg(engineName));
        return false;
    }

    // Convert the QMap<QString, QJsonArray> to a single QJsonArray suitable for the analyzer's save method
    QJsonArray textsToSave;
    for (const QJsonArray &fileTexts : data.values()) {
        for (const QJsonValue &value : fileTexts) {
            textsToSave.append(value);
        }
    }

    if (!analyzer->save(projectPath, textsToSave)) {
        emit errorOccurred(QString("Failed to save strings to project: %1").arg(projectPath));
        return false;
    }
    return true;
}

bool BGADataManager::exportStringsToGameProject(const QString &engineName, const QString &projectPath, const QString &targetDir, const QMap<QString, QJsonArray> &data, bool onlyTranslated)
{
    // Step 0: Filter data if onlyTranslated is true
    QMap<QString, QJsonArray> filteredData;
    
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        const QString &filePath = it.key();
        const QJsonArray &entries = it.value();
        
        if (onlyTranslated) {
            // Check if this file has any translated entries
            bool hasTranslation = false;
            for (const QJsonValue &val : entries) {
                QJsonObject obj = val.toObject();
                QString translation = obj["text"].toString();
                if (!translation.isEmpty()) {
                    hasTranslation = true;
                    break;
                }
            }
            if (hasTranslation) {
                filteredData.insert(filePath, entries);
            }
        } else {
            // Include all files
            filteredData.insert(filePath, entries);
        }
    }
    
    if (filteredData.isEmpty()) {
        emit errorOccurred(tr("No files to export. Please translate some entries first."));
        return false;
    }

    // Step 1: Copy original files to target directory
    QDir projectDir(projectPath);
    QDir targetDirectory(targetDir);

    QSet<QString> filesToProcess;
    for (const QJsonArray &fileTexts : filteredData.values()) {
        for (const QJsonValue &value : fileTexts) {
             filesToProcess.insert(value.toObject()["path"].toString());
        }
    }

    // Copy logic

    for (const QString &sourceFilePath : filesToProcess) {
        QString relativePath = projectDir.relativeFilePath(sourceFilePath);
        QString targetFilePath = targetDirectory.filePath(relativePath);
        QFileInfo targetFileInfo(targetFilePath);

        if (!targetFileInfo.dir().exists()) {
            targetFileInfo.dir().mkpath(".");
        }

        // If deploying in-place (source == target), skip copy/remove to avoid deleting original data
        if (QFileInfo(sourceFilePath).canonicalFilePath() != QFileInfo(targetFilePath).canonicalFilePath()) {
            if (QFile::exists(targetFilePath)) {
                QFile::remove(targetFilePath);
            }

            if (!QFile::copy(sourceFilePath, targetFilePath)) {
                 emit errorOccurred("Failed to copy file: " + sourceFilePath + " to " + targetFilePath);
                 return false;
            }
        }
    }

    // Step 2: Create modified data with paths pointing to the COPIED files
    QJsonArray allTexts;
    for (const QJsonArray &fileTexts : filteredData.values()) {
        for (const QJsonValue &val : fileTexts) {
            QJsonObject obj = val.toObject();
            QString originalPath = obj["path"].toString();
            QString relativePath = projectDir.relativeFilePath(originalPath);
            QString targetFilePath = targetDirectory.filePath(relativePath);
            
            // Normalize separators and use the NEW path for the analyzer to patch
            targetFilePath = QDir::toNativeSeparators(targetFilePath);
            
            obj["path"] = targetFilePath;
            allTexts.append(obj);
        }
    }

    // Step 3: Use Analyzer to patch the COPIED files
    std::unique_ptr<core::IGameAnalyzer> analyzer = core::createAnalyzer(engineName);
    if (!analyzer) {
        emit errorOccurred(QString("Failed to create analyzer for engine: %1").arg(engineName));
        return false;
    }

    // Pass the list of texts with updated paths. 
    // The analyzer->save implementation for RPGM uses the path inside each entry.
    if (!analyzer->save(targetDir, allTexts)) {
        emit errorOccurred(QString("Failed to patch exported project in: %1").arg(targetDir));
        return false;
    }

    return true;
}

QPair<QString, QString> BGADataManager::getScriptDetails(const QString &engineName, const QString &projectPath)
{
    auto analyzer = core::createAnalyzer(engineName);
    if (!analyzer) {
        return qMakePair(QString(), QString());
    }
    
    if (analyzer->canEditScript()) {
        QString path = analyzer->getScriptPath(projectPath);
        QString target = analyzer->getScriptTarget();
        return qMakePair(path, target);
    }
    
    return qMakePair(QString(), QString());
}

bool BGADataManager::createBackup(const QString &projectPath, const QMap<QString, QJsonArray> &data)
{
    QDir projectDir(projectPath);
    QString backupDirPath = projectDir.filePath("_nst_backup");
    QDir backupDir(backupDirPath);
    
    // Create backup directory if it doesn't exist
    if (!backupDir.exists()) {
        if (!QDir().mkpath(backupDirPath)) {
            emit errorOccurred(tr("Failed to create backup directory: %1").arg(backupDirPath));
            return false;
        }
    }
    
    // Collect unique file paths that will be modified
    QSet<QString> filesToBackup;
    for (const QJsonArray &entries : data) {
        for (const QJsonValue &val : entries) {
            QString filePath = val.toObject()["path"].toString();
            if (!filePath.isEmpty() && QFile::exists(filePath)) {
                filesToBackup.insert(filePath);
            }
        }
    }
    
    // Copy each file to backup directory
    int backedUp = 0;
    for (const QString &originalPath : filesToBackup) {
        QString relativePath = projectDir.relativeFilePath(originalPath);
        QString backupPath = backupDir.filePath(relativePath);
        
        // Create parent directories in backup
        QFileInfo backupFileInfo(backupPath);
        if (!backupFileInfo.dir().exists()) {
            backupFileInfo.dir().mkpath(".");
        }
        
        // Skip if already backed up
        if (QFile::exists(backupPath)) {
            backedUp++;
            continue;
        }
        
        // Copy file
        if (QFile::copy(originalPath, backupPath)) {
            backedUp++;
        } else {
            qWarning() << "[NST Backup] Failed to backup:" << originalPath;
        }
    }
    
    // Write backup manifest
    QJsonObject manifest;
    manifest["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    manifest["projectPath"] = projectPath;
    manifest["filesCount"] = backedUp;
    
    QFile manifestFile(backupDir.filePath("manifest.json"));
    if (manifestFile.open(QIODevice::WriteOnly)) {
        manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Compact));
        manifestFile.close();
    }
    
    qDebug() << "[NST Backup] Created backup of" << backedUp << "files in" << backupDirPath;
    return true;
}
