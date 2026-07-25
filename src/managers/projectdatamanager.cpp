#include "projectdatamanager.h"

#include <QFileInfo>
#include <QDebug>
#include <QSet>
#include <QtConcurrent>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QSignalBlocker>
#include <QElapsedTimer>

ProjectDataManager::ProjectDataManager(QObject *parent)
    : QObject(parent)
{
    connect(&m_processingFutureWatcher, &QFutureWatcher<QPair<QMap<QString, QJsonArray>, QStringList>>::finished, this, &ProjectDataManager::onProcessingFinished);
}

QMap<QString, QJsonArray> &ProjectDataManager::getLoadedGameProjectData()
{
    if (m_loadedGameProjectData.isEmpty() && m_db.isOpen()) {
        syncCacheFromDb();
    }
    return m_loadedGameProjectData;
}

QString &ProjectDataManager::getCurrentLoadedFilePath()
{
    return m_currentLoadedFilePath;
}

void ProjectDataManager::syncCacheFromDb()
{
    if (m_db.isOpen()) {
        m_loadedGameProjectData = m_db.getAllDataMap();
    }
}

void ProjectDataManager::clearAllData()
{
    disconnect(&m_processingFutureWatcher, nullptr, this, nullptr);
    m_processingFutureWatcher.cancel();
    
    m_db.close();
    m_loadedGameProjectData.clear();
    m_currentLoadedFilePath.clear();
    
    emit dataCleared();

    connect(&m_processingFutureWatcher, &QFutureWatcher<QPair<QMap<QString, QJsonArray>, QStringList>>::finished, this, &ProjectDataManager::onProcessingFinished);
}

void ProjectDataManager::setProjectPath(const QString &path)
{
    m_projectPath = path;
    if (m_db.isOpen()) {
        m_db.setMeta("projectPath", path);
    }
}

QString ProjectDataManager::getProjectPath() const
{
    if (m_db.isOpen()) {
        QString dbProjPath = m_db.getMeta("projectPath");
        if (!dbProjPath.isEmpty()) return dbProjPath;
    }
    return m_projectPath;
}

void ProjectDataManager::setEngineName(const QString &name)
{
    m_engineName = name;
    if (m_db.isOpen()) {
        m_db.setMeta("engineName", name);
    }
}

QString ProjectDataManager::getEngineName() const
{
    if (m_db.isOpen()) {
        QString dbEngName = m_db.getMeta("engineName");
        if (!dbEngName.isEmpty()) return dbEngName;
    }
    return m_engineName;
}

void ProjectDataManager::onLoadingFinished(const QJsonArray &extractedTextsArray, bool sync)
{
    qDebug() << "ProjectDataManager: onLoadingFinished called with " << extractedTextsArray.size() << " entries. Sync =" << sync;

    auto processFunc = [extractedTextsArray]() {
        QMap<QString, QJsonArray> fileMap;
        QStringList filePaths;
        QSet<QString> uniquePaths;

        for (const QJsonValue &value : extractedTextsArray) {
            QJsonObject obj = value.toObject();
            QString filePath = obj["path"].toString();
            if (filePath.isEmpty()) continue;

            fileMap[filePath].append(obj);
            if (!uniquePaths.contains(filePath)) {
                uniquePaths.insert(filePath);
                filePaths.append(filePath);
            }
        }

        std::sort(filePaths.begin(), filePaths.end(), [](const QString &a, const QString &b) {
            return QFileInfo(a).fileName() < QFileInfo(b).fileName();
        });

        return qMakePair(fileMap, filePaths);
    };

    if (sync) {
        auto result = processFunc();
        m_loadedGameProjectData = result.first;
        if (m_db.isOpen()) {
            m_db.insertOrReplaceAll(m_loadedGameProjectData);
        }
        emit fileListUpdated(result.second);
        emit processingFinished();
    } else {
        QFuture<QPair<QMap<QString, QJsonArray>, QStringList>> future = QtConcurrent::run(processFunc);
        m_processingFutureWatcher.setFuture(future);
    }
}

void ProjectDataManager::mergeLoadingFinished(const QJsonArray &newExtractedTextsArray, bool sync)
{
    qDebug() << "ProjectDataManager: mergeLoadingFinished called with " << newExtractedTextsArray.size() << " entries. Sync =" << sync;

    QMap<QString, QString> translationMemory;
    for (auto it = m_loadedGameProjectData.begin(); it != m_loadedGameProjectData.end(); ++it) {
        const QJsonArray &arr = it.value();
        for (const QJsonValue &val : arr) {
            QJsonObject obj = val.toObject();
            QString source = obj["source"].toString();
            QString key = obj["key"].toString();
            QString translation = obj["text"].toString();
            
            if (!translation.isEmpty()) {
                translationMemory.insert(source + "|" + key, translation);
            }
        }
    }

    auto processFunc = [newExtractedTextsArray, translationMemory]() {
        QMap<QString, QJsonArray> fileMap;
        QStringList filePaths;
        QSet<QString> uniquePaths;

        for (const QJsonValue &value : newExtractedTextsArray) {
            QJsonObject obj = value.toObject();
            QString filePath = obj["path"].toString();
            if (filePath.isEmpty()) continue;

            QString source = obj["source"].toString();
            QString key = obj["key"].toString();
            
            QString tmKey = source + "|" + key;
            if (translationMemory.contains(tmKey)) {
                obj["text"] = translationMemory.value(tmKey);
            }

            fileMap[filePath].append(obj);
            if (!uniquePaths.contains(filePath)) {
                uniquePaths.insert(filePath);
                filePaths.append(filePath);
            }
        }

        std::sort(filePaths.begin(), filePaths.end(), [](const QString &a, const QString &b) {
            return QFileInfo(a).fileName() < QFileInfo(b).fileName();
        });

        return qMakePair(fileMap, filePaths);
    };

    if (sync) {
        auto result = processFunc();
        m_loadedGameProjectData = result.first;
        if (m_db.isOpen()) {
            m_db.insertOrReplaceAll(m_loadedGameProjectData);
        }
        emit fileListUpdated(result.second);
        emit processingFinished();
    } else {
        QFuture<QPair<QMap<QString, QJsonArray>, QStringList>> future = QtConcurrent::run(processFunc);
        m_processingFutureWatcher.setFuture(future);
    }
}

void ProjectDataManager::onProcessingFinished()
{
    qDebug() << "ProjectDataManager: Background processing finished.";
    QPair<QMap<QString, QJsonArray>, QStringList> result = m_processingFutureWatcher.result();
    m_loadedGameProjectData = result.first;
    
    if (m_db.isOpen()) {
        m_db.insertOrReplaceAll(m_loadedGameProjectData);
    }

    emit fileListUpdated(result.second);

    qDebug() << "ProjectDataManager: Data loaded. Emitting processingFinished signal.";
    emit processingFinished();
}

void ProjectDataManager::onFileSelected(const QModelIndex &index)
{
    QString fullFilePath = index.data(Qt::UserRole).toString();
    if (!fullFilePath.isEmpty()) {
        selectFile(fullFilePath);
    }
}

void ProjectDataManager::selectFile(const QString &filePath)
{
    m_currentLoadedFilePath = filePath;
    if (m_db.isOpen()) {
        QJsonArray entries = m_db.getEntriesForFile(filePath);
        emit fileSelected(filePath, entries);
    } else if (m_loadedGameProjectData.contains(filePath)) {
        emit fileSelected(filePath, m_loadedGameProjectData.value(filePath));
    }
}

void ProjectDataManager::updateTranslation(const QString &source, const QString &translation, const QString &filePath)
{
    QString targetPath = filePath.isEmpty() ? m_currentLoadedFilePath : filePath;
    if (targetPath.isEmpty()) return;

    if (m_db.isOpen()) {
        m_db.updateTranslation(targetPath, source, translation);
    }

    if (m_loadedGameProjectData.contains(targetPath)) {
        QJsonArray textsArray = m_loadedGameProjectData[targetPath];
        bool modified = false;
        for (int i = 0; i < textsArray.size(); ++i) {
            QJsonObject obj = textsArray.at(i).toObject();
            if (obj["source"].toString() == source) {
                obj["text"] = translation;
                textsArray.replace(i, obj);
                modified = true;
            }
        }
        if (modified) {
            m_loadedGameProjectData[targetPath] = textsArray;
        }
    }

    emit translationUpdated(targetPath, source, translation);
}

void ProjectDataManager::saveGameProject()
{
    QMapIterator<QString, QJsonArray> i(m_loadedGameProjectData);
    while (i.hasNext()) {
        i.next();
        QString filePath = i.key();
        QJsonArray data = i.value();

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(data);
            file.write(doc.toJson());
            file.close();
        } else {
            qWarning() << "Failed to save file:" << filePath;
        }
    }
}

void ProjectDataManager::setHideCompleted(bool hide)
{
    m_hideCompleted = hide;
}

void ProjectDataManager::exportGameProject(const QString &targetDir)
{
    Q_UNUSED(targetDir)
}

bool ProjectDataManager::saveTranslationWorkspace(const QString &filePath)
{
    if (filePath.isEmpty()) return false;

    if (!m_db.isOpen() || m_db.currentDatabasePath() != filePath) {
        // Create/open database at target file path
        if (!m_db.create(filePath)) {
            qWarning() << "ProjectDataManager: Failed to create SQLite workspace at:" << filePath;
            return false;
        }
        m_db.setMeta("projectPath", m_projectPath);
        m_db.setMeta("engineName", m_engineName);
        m_db.insertOrReplaceAll(m_loadedGameProjectData);
    } else {
        // DB is already open at target path, update metadata
        m_db.setMeta("projectPath", m_projectPath);
        m_db.setMeta("engineName", m_engineName);
    }

    qDebug() << "ProjectDataManager: Successfully saved workspace to SQLite database:" << filePath;
    return true;
}

bool ProjectDataManager::loadTranslationWorkspace(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        qWarning() << "ProjectDataManager: Workspace file does not exist:" << filePath;
        return false;
    }

    QElapsedTimer loadTimer;
    loadTimer.start();

    if (NstDatabase::isSQLiteFile(filePath)) {
        // Modern SQLite workspace
        if (!m_db.open(filePath)) {
            qWarning() << "ProjectDataManager: Failed to open SQLite workspace:" << filePath;
            return false;
        }
    } else {
        // Legacy JSON workspace — auto-migrate to SQLite
        qDebug() << "ProjectDataManager: Detecting legacy JSON workspace, converting to SQLite database:" << filePath;

        // Backup original JSON file
        QString backupPath = filePath + ".json.bak";
        if (!QFile::exists(backupPath)) {
            QFile::copy(filePath, backupPath);
        }

        if (!m_db.create(filePath)) {
            qWarning() << "ProjectDataManager: Failed to create SQLite database for migration:" << filePath;
            return false;
        }

        if (!m_db.importFromJson(backupPath)) {
            qWarning() << "ProjectDataManager: Failed to import legacy JSON into SQLite:" << filePath;
            return false;
        }
    }

    m_projectPath = m_db.getMeta("projectPath");
    m_engineName = m_db.getMeta("engineName");

    // Skip synchronous in-memory full cache dump; SQLite indexes getList and getEntries instantly on demand.

    QStringList files = m_db.getFileList();
    std::sort(files.begin(), files.end(), [](const QString &a, const QString &b) {
         return QFileInfo(a).fileName() < QFileInfo(b).fileName();
    });

    emit fileListUpdated(files);
    qInfo().noquote() << QString("[PERF] Workspace DB '%1' loaded (%2 files) in %3 ms")
                        .arg(QFileInfo(filePath).fileName())
                        .arg(files.size())
                        .arg(loadTimer.elapsed());
    return true;
}
