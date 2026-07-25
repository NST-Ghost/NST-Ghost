#ifndef PROJECTDATAMANAGER_H
#define PROJECTDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QJsonArray>
#include <QJsonObject>
#include <QFutureWatcher>
#include <QPair>
#include <QSet>
#include <QStringList>
#include <QModelIndex>

#include "../db/nstdatabase.h"

class ProjectDataManager : public QObject
{
    Q_OBJECT
public:
    explicit ProjectDataManager(QObject *parent = nullptr);

    QMap<QString, QJsonArray> &getLoadedGameProjectData();
    QString &getCurrentLoadedFilePath();

    NstDatabase &database() { return m_db; }
    const NstDatabase &database() const { return m_db; }

    void clearAllData();

    void updateTranslation(const QString &source, const QString &translation, const QString &filePath = QString());
    void saveGameProject();
    void exportGameProject(const QString &targetDir);
    void setProjectPath(const QString &path);
    void setHideCompleted(bool hide);
    bool hideCompleted() const { return m_hideCompleted; }
    QString getProjectPath() const;
    void setEngineName(const QString &name);
    QString getEngineName() const;

    bool saveTranslationWorkspace(const QString &filePath);
    bool loadTranslationWorkspace(const QString &filePath);

    // Headless-compatible file selection
    void selectFile(const QString &filePath);

public slots:
    void onLoadingFinished(const QJsonArray &extractedTextsArray, bool sync = false);
    void mergeLoadingFinished(const QJsonArray &newExtractedTextsArray, bool sync = false);
    void onFileSelected(const QModelIndex &index);

private slots:
    void onProcessingFinished();

signals:
    void processingFinished();
    void fileListUpdated(const QStringList &filePaths);
    void fileSelected(const QString &filePath, const QJsonArray &entries);
    void translationUpdated(const QString &filePath, const QString &source, const QString &translation);
    void dataCleared();

private:
    void syncCacheFromDb();

    NstDatabase m_db;
    QMap<QString, QJsonArray> m_loadedGameProjectData;
    QString m_currentLoadedFilePath;
    QFutureWatcher<QPair<QMap<QString, QJsonArray>, QStringList>> m_processingFutureWatcher;
    bool m_hideCompleted = false;
    QString m_projectPath;
    QString m_engineName;
};

#endif // PROJECTDATAMANAGER_H
