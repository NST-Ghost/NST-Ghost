#ifndef PROJECTREGISTRY_H
#define PROJECTREGISTRY_H

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QJsonObject>

/**
 * @class ProjectRegistry
 * @brief Manages a central registry of NST translation project files.
 *
 * Stores path references (not copies) to .nst workspace files along with
 * cached metadata (engine name, display name, modification date, progress).
 * The registry is persisted as a JSON file in the user's app data directory.
 */
class ProjectRegistry : public QObject
{
    Q_OBJECT
public:
    struct ProjectEntry {
        QString filePath;           ///< Absolute path to .nst file
        QString displayName;        ///< User-friendly project name
        QString engineName;         ///< Engine type: RPGM, UNITY, RENPY
        QString projectPath;        ///< Original game directory
        QDateTime lastModified;     ///< Last modified timestamp (from filesystem)
        int fileCount = 0;          ///< Number of translatable files
        int translatedPercent = 0;  ///< Translation progress (0-100)

        QJsonObject toJson() const;
        static ProjectEntry fromJson(const QJsonObject &obj);
    };

    explicit ProjectRegistry(QObject *parent = nullptr);

    /// Returns all registered projects, sorted by last modified (newest first).
    QList<ProjectEntry> getAllProjects() const;

    /// Register or update a project entry by reading the .nst file metadata.
    /// Returns true if the project was successfully registered.
    bool registerProject(const QString &filePath);

    /// Remove a project from the registry (does NOT delete the .nst file).
    void removeProject(const QString &filePath);

    /// Rename a project's display name.
    void renameProject(const QString &filePath, const QString &newName);

    /// Check if a project is already registered.
    bool projectExists(const QString &filePath) const;

    /// Persist the registry to disk.
    void save() const;

    /// Load the registry from disk.
    void load();

    /// Refresh metadata for all projects (re-reads filesystem info).
    void refreshAll();

signals:
    void projectsChanged();

private:
    /// Returns the path to the registry JSON file.
    QString registryFilePath() const;

    /// Returns the app data directory for NST projects.
    QString registryDir() const;

    /// Read .nst file metadata without fully loading the workspace.
    ProjectEntry readProjectMetadata(const QString &filePath) const;

    /// Calculate translation progress from project data.
    static int calculateProgress(const QJsonObject &dataObj);

    QList<ProjectEntry> m_projects;
};

#endif // PROJECTREGISTRY_H
