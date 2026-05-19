#ifndef PROJECTDATAMANAGER_H
#define PROJECTDATAMANAGER_H

#include <QObject>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QMap>
#include <QJsonArray>
#include <QJsonObject>
#include <QFutureWatcher>
#include <QPair>
#include <QSet>
#include <QStringList>

class ProjectDataManager : public QObject
{
    Q_OBJECT
public:
    explicit ProjectDataManager(QStandardItemModel *fileListModel = nullptr, QStandardItemModel *translationModel = nullptr, QObject *parent = nullptr);

    QMap<QString, QJsonArray> &getLoadedGameProjectData();
    QString &getCurrentLoadedFilePath();

    void clearAllData();

    void updateTranslation(const QString &source, const QString &translation, const QString &filePath = QString());
    void saveGameProject();
    void exportGameProject(const QString &targetDir);
    void setProjectPath(const QString &path);
    void setHideCompleted(bool hide);
    QString getProjectPath() const { return m_projectPath; }
    void setEngineName(const QString &name) { m_engineName = name; }
    QString getEngineName() const { return m_engineName; }

    bool saveTranslationWorkspace(const QString &filePath);
    bool loadTranslationWorkspace(const QString &filePath);

public slots:
    void onLoadingFinished(const QJsonArray &extractedTextsArray, bool sync = false);
    void mergeLoadingFinished(const QJsonArray &newExtractedTextsArray, bool sync = false);
    void onFileSelected(const QModelIndex &index);

private slots:
    void onProcessingFinished();
    

signals:
    void processingFinished();

private:
    QStandardItemModel *m_fileListModel;
    QStandardItemModel *m_translationModel;
    QMap<QString, QJsonArray> m_loadedGameProjectData;
    QString m_currentLoadedFilePath;
    QFutureWatcher<QPair<QMap<QString, QJsonArray>, QStringList>> m_processingFutureWatcher;
    bool m_hideCompleted = false;
    QString m_projectPath;
    QString m_engineName;
};

#endif // PROJECTDATAMANAGER_H
