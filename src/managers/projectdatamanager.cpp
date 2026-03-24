#include "projectdatamanager.h"

#include <QFileInfo>
#include <QDebug>
#include <QSet>
#include <QtConcurrent>
#include <QJsonDocument>
#include <QFile>
#include <QDir>

ProjectDataManager::ProjectDataManager(QStandardItemModel *fileListModel, QStandardItemModel *translationModel, QObject *parent)
    : QObject(parent)
    , m_fileListModel(fileListModel)
    , m_translationModel(translationModel)
{
    connect(&m_processingFutureWatcher, &QFutureWatcher<QPair<QMap<QString, QJsonArray>, QStringList>>::finished, this, &ProjectDataManager::onProcessingFinished);
}

QMap<QString, QJsonArray> &ProjectDataManager::getLoadedGameProjectData()
{
    return m_loadedGameProjectData;
}

QString &ProjectDataManager::getCurrentLoadedFilePath()
{
    return m_currentLoadedFilePath;
}

void ProjectDataManager::clearAllData()
{
    // Disconnect to signal to prevent handling "finished" from the cancelled future
    disconnect(&m_processingFutureWatcher, nullptr, this, nullptr);
    m_processingFutureWatcher.cancel();
    
    // Do NOT set empty future as it might cause Qt internal crash with complex types
    // m_processingFutureWatcher.setFuture(...); 

    m_loadedGameProjectData.clear();
    m_currentLoadedFilePath.clear();
    if (m_fileListModel) m_fileListModel->clear();
    if (m_translationModel) m_translationModel->clear();

    // Reconnect the signal for the future usage
    connect(&m_processingFutureWatcher, &QFutureWatcher<QPair<QMap<QString, QJsonArray>, QStringList>>::finished, this, &ProjectDataManager::onProcessingFinished);
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
        if (m_fileListModel) {
            m_fileListModel->clear();
            for (const QString &path : result.second) {
                QStandardItem *item = new QStandardItem(QFileInfo(path).fileName());
                item->setData(path, Qt::UserRole);
                m_fileListModel->appendRow(item);
            }
        }
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
    
    m_fileListModel->clear();
    for (const QString &path : result.second) {
        QStandardItem *item = new QStandardItem(QFileInfo(path).fileName());
        item->setData(path, Qt::UserRole);
        m_fileListModel->appendRow(item);
    }

    qDebug() << "ProjectDataManager: Models updated. Emitting processingFinished signal.";
    emit processingFinished();
}


void ProjectDataManager::onFileSelected(const QModelIndex &index)
{
    QStandardItem *item = m_fileListModel->itemFromIndex(index);
    if (!item) return;
    
    QString fullFilePath = item->data(Qt::UserRole).toString();

    m_translationModel->clear();
    m_translationModel->setHorizontalHeaderLabels(QStringList() << "Context" << "Source Text" << "Translation");

    m_currentLoadedFilePath = fullFilePath;

    if (m_loadedGameProjectData.contains(fullFilePath)) {
        const QJsonArray &textsArray = m_loadedGameProjectData.value(fullFilePath);
        if (textsArray.isEmpty()) return;

        // Temporarily disable updates for performance
        // This will be handled by MainWindow's QTableView directly
        // For now, we'll just populate the model

        for (const QJsonValue &value : textsArray) {
            QJsonObject obj = value.toObject();
            QString source = obj["source"].toString();
            QString translation = obj["text"].toString();
            QString key = obj["key"].toString();
            QString warning = obj["warning"].toString();

            QStandardItem *contextItem = new QStandardItem(key);
            contextItem->setEditable(false); // Context should be read-only
            contextItem->setForeground(QBrush(QColor(150, 150, 150))); // Grey out context text
            
            if (!warning.isEmpty()) {
                contextItem->setIcon(QIcon::fromTheme("dialog-warning")); // Or built-in standard icon
                contextItem->setToolTip("Warning: " + warning);
                contextItem->setData(warning, Qt::UserRole + 2); // Store warning data
            }

            QStandardItem *sourceItem = new QStandardItem(source);
            QStandardItem *transItem = new QStandardItem(translation);

            // Store key in source item as well for backward compatibility if needed, 
            // but now it's visible in the first column.
            sourceItem->setData(key, Qt::UserRole + 1);

            if (m_hideCompleted && !translation.isEmpty()) {
                 continue;
            }

            m_translationModel->appendRow(QList<QStandardItem*>() << contextItem << sourceItem << transItem);
        }
    }
}

void ProjectDataManager::updateTranslation(const QString &source, const QString &translation, const QString &filePath)
{
    QString targetPath = filePath.isEmpty() ? m_currentLoadedFilePath : filePath;
    if (targetPath.isEmpty() || !m_loadedGameProjectData.contains(targetPath))
        return;

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
        
        // Also update UI if model is present and we're looking at this file
        if (m_translationModel && targetPath == m_currentLoadedFilePath) {
            for (int i = 0; i < m_translationModel->rowCount(); ++i) {
                if (m_translationModel->data(m_translationModel->index(i, 1)).toString() == source) {
                    m_translationModel->setData(m_translationModel->index(i, 2), translation);
                }
            }
        }
    }
}

void ProjectDataManager::saveGameProject()
{
    // Iterate over all loaded files and save them back to disk
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

void ProjectDataManager::setProjectPath(const QString &path)
{
    m_projectPath = path;
}

void ProjectDataManager::exportGameProject(const QString &targetDir)
{
    // Deprecated or delegated to BGA via FileTranslationWidget
    // Keeping empty or removing implementation if not used.
    // For now, let's leave it but it shouldn't be called directly without BGA logic.
}

bool ProjectDataManager::saveTranslationWorkspace(const QString &filePath)
{
    QJsonObject rootObj;
    rootObj.insert("projectPath", m_projectPath);
    rootObj.insert("engineName", m_engineName);
    
    QJsonObject dataObj;
    QMapIterator<QString, QJsonArray> i(m_loadedGameProjectData);
    while (i.hasNext()) {
        i.next();
        dataObj.insert(i.key(), i.value());
    }
    rootObj.insert("data", dataObj);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open workspace file for writing:" << filePath;
        return false;
    }

    QJsonDocument doc(rootObj);
    file.write(doc.toJson());
    file.close();
    return true;
}

bool ProjectDataManager::loadTranslationWorkspace(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open workspace file for reading:" << filePath;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid workspace file format.";
        return false;
    }

    QJsonObject rootObj = doc.object();
    m_projectPath = rootObj["projectPath"].toString();
    m_engineName = rootObj["engineName"].toString();
    
    QJsonObject dataObj = rootObj["data"].toObject();
    m_loadedGameProjectData.clear();
    
    for (auto it = dataObj.begin(); it != dataObj.end(); ++it) {
        if (it.value().isArray()) {
            m_loadedGameProjectData.insert(it.key(), it.value().toArray());
        }
    }

    // Refresh models
    m_fileListModel->clear();
    m_translationModel->clear();
    
    // Populate file list sorted by name
    QStringList files = m_loadedGameProjectData.keys();
    std::sort(files.begin(), files.end(), [](const QString &a, const QString &b) {
         return QFileInfo(a).fileName() < QFileInfo(b).fileName();
    });

    for (const QString &path : files) {
        QStandardItem *item = new QStandardItem(QFileInfo(path).fileName());
        item->setData(path, Qt::UserRole);
        m_fileListModel->appendRow(item);
    }
    
    return true;
}
