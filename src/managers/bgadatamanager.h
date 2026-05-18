#ifndef BGADATAMANAGER_H
#define BGADATAMANAGER_H

#include <QObject>
#include <QStringList>
#include <QStandardItemModel>
#include <QMap>
#include <QPair>
#include <QJsonArray>

// Include BGACore headers
#include <core/gameanalyzer.h>
#include <core/analyzerfactory.h>

class BGADataManager : public QObject
{
    Q_OBJECT
public:
    explicit BGADataManager(QObject *parent = nullptr);

    QStringList getAvailableAnalyzers() const;
    QJsonArray loadStringsFromGameProject(const QString &engineName, const QString &projectPath);
    bool saveStringsToGameProject(const QString &engineName, const QString &projectPath, const QMap<QString, QJsonArray> &data, const QString &targetLanguage = QString());
    bool exportStringsToGameProject(const QString &engineName, const QString &projectPath, const QString &targetDir, const QMap<QString, QJsonArray> &data, bool onlyTranslated = true, const QString &targetLanguage = QString());
    QPair<QString, QString> getScriptDetails(const QString &engineName, const QString &projectPath);

    QJsonArray loadedFonts() const;
    
    // Backup functionality for CLI deploy
    bool createBackup(const QString &projectPath, const QMap<QString, QJsonArray> &data);

signals:
    void errorOccurred(const QString &message);
    void fontsLoaded(const QJsonArray &fonts);
    void progressUpdated(int percentage, const QString &message);
    void loadingFinished();

private:
    QJsonArray m_loadedFonts;};

#endif // BGADATAMANAGER_H
