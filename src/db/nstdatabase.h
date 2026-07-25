/**
 * @file nstdatabase.h
 * @brief SQLite Storage Engine for NST (Novel / Game Translation tool)
 * 
 * ============================================================================
 * [NOTE] INSTRUCTIONS FOR AI / LLM ASSISTANTS & DEVELOPERS (SCHEMA MIGRATION GUIDE)
 * ============================================================================
 * 
 * When adding new columns, tables, or altering the database schema in future:
 * 
 * 1. INCREMENT SCHEMA VERSION:
 *    - Increase `CURRENT_SCHEMA_VERSION` in `nstdatabase.h` (e.g. from 1 to 2).
 * 
 * 2. UPDATE FRESH DB CREATION (`createTablesAndTriggers` in nstdatabase.cpp):
 *    - Update `createTablesAndTriggers()` so that newly created databases get the latest schema directly.
 * 
 * 3. ADD MIGRATION STEP (`runMigration` in nstdatabase.cpp):
 *    - Add migration logic inside `runMigration(int fromVersion, int toVersion)` for incremental upgrades.
 *    - Example (v1 -> v2):
 *      ```cpp
 *      if (fromVersion == 1 && toVersion == 2) {
 *          QSqlQuery q(m_db);
 *          if (!q.exec("ALTER TABLE entries ADD COLUMN status TEXT DEFAULT 'draft';")) return false;
 *          return true;
 *      }
 *      ```
 * 
 * 4. SAFETY RULES FOR MIGRATION:
 *    - NEVER drop or erase existing user columns/tables containing translation data!
 *    - Use `ALTER TABLE ... ADD COLUMN ... DEFAULT ...` when adding fields.
 *    - Keep migration incremental (v1 -> v2 -> v3) so users skipping app versions update seamlessly.
 * ============================================================================
 */

#ifndef NSTDATABASE_H
#define NSTDATABASE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

class NstDatabase : public QObject
{
    Q_OBJECT
public:
    static constexpr int CURRENT_SCHEMA_VERSION = 1;

    explicit NstDatabase(QObject *parent = nullptr);
    ~NstDatabase();

    // Database Lifecycle
    bool open(const QString &dbPath);
    bool create(const QString &dbPath);
    void close();
    bool isOpen() const;
    QString currentDatabasePath() const { return m_dbPath; }

    // Static helper to check if a file is a valid SQLite database
    static bool isSQLiteFile(const QString &filePath);

    // Metadata
    QString getMeta(const QString &key, const QString &defaultValue = QString()) const;
    bool setMeta(const QString &key, const QString &value);
    int getSchemaVersion() const;
    void getStats(int &totalEntries, int &translatedEntries) const;

    // CRUD & Data Access
    QStringList getFileList() const;
    QJsonArray getEntriesForFile(const QString &filePath) const;
    QMap<QString, QJsonArray> getAllDataMap() const;
    QJsonArray getUntranslatedEntries(const QStringList &filePaths = QStringList()) const;

    bool insertEntries(const QJsonArray &entries);
    bool insertOrReplaceAll(const QMap<QString, QJsonArray> &dataMap);
    bool updateTranslation(const QString &filePath, const QString &source, const QString &translation);
    bool batchUpdateTranslations(const QList<QJsonObject> &updates);
    bool clearAllData();

    // Search (FTS5)
    QJsonArray searchFTS(const QString &searchTerm) const;

    // JSON Import/Export (Backward Compatibility)
    bool importFromJson(const QString &jsonFilePath, const QString &projectPath = QString(), const QString &engineName = QString());
    bool exportToJson(const QString &jsonFilePath) const;

private:
    bool createTablesAndTriggers();
    bool checkAndMigrateSchema();
    bool runMigration(int fromVersion, int toVersion);

    QSqlDatabase m_db;
    QString m_dbPath;
    QString m_connectionName;
};

#endif // NSTDATABASE_H
