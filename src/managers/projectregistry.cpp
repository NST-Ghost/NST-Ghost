#include "projectregistry.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>
#include "../db/nstdatabase.h"

/* =========================================================================
 *  ProjectEntry serialization
 * ========================================================================= */

QJsonObject ProjectRegistry::ProjectEntry::toJson() const
{
    QJsonObject obj;
    obj["filePath"] = filePath;
    obj["displayName"] = displayName;
    obj["engineName"] = engineName;
    obj["projectPath"] = projectPath;
    obj["lastModified"] = lastModified.toString(Qt::ISODate);
    obj["fileCount"] = fileCount;
    obj["translatedPercent"] = translatedPercent;
    return obj;
}

ProjectRegistry::ProjectEntry ProjectRegistry::ProjectEntry::fromJson(const QJsonObject &obj)
{
    ProjectEntry entry;
    entry.filePath = obj["filePath"].toString();
    entry.displayName = obj["displayName"].toString();
    entry.engineName = obj["engineName"].toString();
    entry.projectPath = obj["projectPath"].toString();
    entry.lastModified = QDateTime::fromString(obj["lastModified"].toString(), Qt::ISODate);
    entry.fileCount = obj["fileCount"].toInt(0);
    entry.translatedPercent = obj["translatedPercent"].toInt(0);
    return entry;
}

/* =========================================================================
 *  Constructor
 * ========================================================================= */

ProjectRegistry::ProjectRegistry(QObject *parent)
    : QObject(parent)
{
    load();
}

/* =========================================================================
 *  Public API
 * ========================================================================= */

QList<ProjectRegistry::ProjectEntry> ProjectRegistry::getAllProjects() const
{
    return m_projects;
}

bool ProjectRegistry::registerProject(const QString &filePath)
{
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) {
        qWarning() << "ProjectRegistry: File does not exist:" << filePath;
        return false;
    }

    QString absPath = fi.absoluteFilePath();

    // Read metadata from the .nst file
    ProjectEntry entry = readProjectMetadata(absPath);
    if (entry.filePath.isEmpty()) {
        qWarning() << "ProjectRegistry: Failed to read metadata from:" << absPath;
        return false;
    }

    // Check if already registered — update in place
    for (int i = 0; i < m_projects.size(); ++i) {
        if (m_projects[i].filePath == absPath) {
            // Preserve user-set display name if it was customized
            QString existingName = m_projects[i].displayName;
            m_projects[i] = entry;
            if (!existingName.isEmpty() && existingName != QFileInfo(absPath).baseName()) {
                m_projects[i].displayName = existingName;
            }
            save();
            emit projectsChanged();
            return true;
        }
    }

    // New entry
    m_projects.prepend(entry);
    save();
    emit projectsChanged();
    return true;
}

void ProjectRegistry::removeProject(const QString &filePath)
{
    QString absPath = QFileInfo(filePath).absoluteFilePath();
    for (int i = 0; i < m_projects.size(); ++i) {
        if (m_projects[i].filePath == absPath) {
            m_projects.removeAt(i);
            save();
            emit projectsChanged();
            return;
        }
    }
}

void ProjectRegistry::renameProject(const QString &filePath, const QString &newName)
{
    QString absPath = QFileInfo(filePath).absoluteFilePath();
    for (int i = 0; i < m_projects.size(); ++i) {
        if (m_projects[i].filePath == absPath) {
            m_projects[i].displayName = newName;
            save();
            emit projectsChanged();
            return;
        }
    }
}

bool ProjectRegistry::projectExists(const QString &filePath) const
{
    QString absPath = QFileInfo(filePath).absoluteFilePath();
    for (const ProjectEntry &entry : m_projects) {
        if (entry.filePath == absPath) {
            return true;
        }
    }
    return false;
}

void ProjectRegistry::refreshAll()
{
    bool changed = false;
    for (int i = m_projects.size() - 1; i >= 0; --i) {
        QFileInfo fi(m_projects[i].filePath);
        if (!fi.exists()) {
            // File was deleted — remove from registry
            m_projects.removeAt(i);
            changed = true;
            continue;
        }
        // Update modification time
        m_projects[i].lastModified = fi.lastModified();
    }

    // Sort by last modified (newest first)
    std::sort(m_projects.begin(), m_projects.end(),
              [](const ProjectEntry &a, const ProjectEntry &b) {
                  return a.lastModified > b.lastModified;
              });

    if (changed) {
        save();
    }
    emit projectsChanged();
}

/* =========================================================================
 *  Persistence
 * ========================================================================= */

void ProjectRegistry::save() const
{
    QString dirPath = registryDir();
    QDir().mkpath(dirPath);

    QJsonArray arr;
    for (const ProjectEntry &entry : m_projects) {
        arr.append(entry.toJson());
    }

    QJsonObject root;
    root["version"] = 1;
    root["projects"] = arr;

    QFile file(registryFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    } else {
        qWarning() << "ProjectRegistry: Failed to save registry to:" << registryFilePath();
    }
}

void ProjectRegistry::load()
{
    m_projects.clear();

    QFile file(registryFilePath());
    if (!file.exists()) return;

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ProjectRegistry: Failed to open registry:" << registryFilePath();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    QJsonArray arr = root["projects"].toArray();

    for (const QJsonValue &val : arr) {
        if (val.isObject()) {
            ProjectEntry entry = ProjectEntry::fromJson(val.toObject());
            if (!entry.filePath.isEmpty()) {
                m_projects.append(entry);
            }
        }
    }

    qDebug() << "ProjectRegistry: Loaded" << m_projects.size() << "projects";
}

/* =========================================================================
 *  Private helpers
 * ========================================================================= */

QString ProjectRegistry::registryDir() const
{
#ifdef Q_OS_WIN
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/projects";
#else
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/projects";
#endif
}

QString ProjectRegistry::registryFilePath() const
{
    return registryDir() + "/projects.json";
}

ProjectRegistry::ProjectEntry ProjectRegistry::readProjectMetadata(const QString &filePath) const
{
    ProjectEntry entry;
    entry.filePath = filePath;

    QFileInfo fi(filePath);
    entry.lastModified = fi.lastModified();
    entry.displayName = fi.baseName();  // Default name from filename

    if (NstDatabase::isSQLiteFile(filePath)) {
        NstDatabase db;
        if (db.open(filePath)) {
            entry.engineName = db.getMeta("engineName");
            entry.projectPath = db.getMeta("projectPath");
            entry.fileCount = db.getFileList().size();
            int total = 0, translated = 0;
            db.getStats(total, translated);
            entry.translatedPercent = total > 0 ? qRound(100.0 * translated / total) : 0;
            db.close();
        }
        return entry;
    }

    // Read legacy .nst JSON to extract metadata
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return entry;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return entry;

    QJsonObject root = doc.object();
    entry.engineName = root["engineName"].toString();
    entry.projectPath = root["projectPath"].toString();

    // Count files and calculate progress from the data object
    QJsonObject dataObj = root["data"].toObject();
    entry.fileCount = dataObj.keys().size();
    entry.translatedPercent = calculateProgress(dataObj);

    return entry;
}

int ProjectRegistry::calculateProgress(const QJsonObject &dataObj)
{
    int totalEntries = 0;
    int translatedEntries = 0;

    for (auto it = dataObj.begin(); it != dataObj.end(); ++it) {
        if (!it.value().isArray()) continue;
        QJsonArray arr = it.value().toArray();
        for (const QJsonValue &val : arr) {
            QJsonObject obj = val.toObject();
            totalEntries++;
            QString text = obj["text"].toString();
            if (!text.isEmpty()) {
                translatedEntries++;
            }
        }
    }

    if (totalEntries == 0) return 0;
    return qRound(100.0 * translatedEntries / totalEntries);
}
