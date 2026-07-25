#include "nstdatabase.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUuid>
#include <QDebug>
#include <QDateTime>

NstDatabase::NstDatabase(QObject *parent)
    : QObject(parent)
{
    m_connectionName = QString("NST_DB_CONN_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

NstDatabase::~NstDatabase()
{
    close();
}

bool NstDatabase::isSQLiteFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray header = file.read(16);
    file.close();
    return header.startsWith("SQLite format 3");
}

bool NstDatabase::open(const QString &dbPath)
{
    if (isOpen()) {
        close();
    }

    m_dbPath = dbPath;
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "NstDatabase: Failed to open database at" << dbPath << ":" << m_db.lastError().text();
        return false;
    }

    // Enable WAL mode & performance optimization pragmas
    QSqlQuery pragmaQuery(m_db);
    pragmaQuery.exec("PRAGMA journal_mode=WAL;");
    pragmaQuery.exec("PRAGMA synchronous=NORMAL;");
    pragmaQuery.exec("PRAGMA temp_store=MEMORY;");
    pragmaQuery.exec("PRAGMA cache_size=-16000;"); // 16MB in-memory page cache
    pragmaQuery.exec("PRAGMA foreign_keys=ON;");

    if (!checkAndMigrateSchema()) {
        qWarning() << "NstDatabase: Schema migration failed for" << dbPath;
        return false;
    }

    qDebug() << "NstDatabase: Successfully opened database at" << dbPath;
    return true;
}

bool NstDatabase::create(const QString &dbPath)
{
    if (isOpen()) {
        close();
    }

    // Remove file if it already exists to start clean
    if (QFile::exists(dbPath)) {
        QFile::remove(dbPath);
    }

    m_dbPath = dbPath;
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "NstDatabase: Failed to create database at" << dbPath << ":" << m_db.lastError().text();
        return false;
    }

    QSqlQuery pragmaQuery(m_db);
    pragmaQuery.exec("PRAGMA journal_mode=WAL;");
    pragmaQuery.exec("PRAGMA synchronous=NORMAL;");
    pragmaQuery.exec("PRAGMA foreign_keys=ON;");

    if (!createTablesAndTriggers()) {
        qWarning() << "NstDatabase: Failed to create tables and triggers in" << dbPath;
        return false;
    }

    setMeta("schema_version", QString::number(CURRENT_SCHEMA_VERSION));
    setMeta("created_at", QDateTime::currentDateTime().toString(Qt::ISODate));

    qDebug() << "NstDatabase: Successfully created new database at" << dbPath;
    return true;
}

void NstDatabase::close()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    m_db = QSqlDatabase();
    if (!m_connectionName.isEmpty() && QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
    m_dbPath.clear();
}

bool NstDatabase::isOpen() const
{
    return m_db.isOpen();
}

bool NstDatabase::createTablesAndTriggers()
{
    if (!isOpen()) return false;

    QSqlQuery q(m_db);

    // 1. Meta Table
    if (!q.exec("CREATE TABLE IF NOT EXISTS project_meta ("
                "  key TEXT PRIMARY KEY, "
                "  value TEXT NOT NULL"
                ");")) {
        qWarning() << "NstDatabase: Error creating project_meta table:" << q.lastError().text();
        return false;
    }

    // 2. Entries Table
    if (!q.exec("CREATE TABLE IF NOT EXISTS entries ("
                "  id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "  file_path TEXT NOT NULL, "
                "  key TEXT NOT NULL, "
                "  source TEXT NOT NULL, "
                "  text TEXT NOT NULL DEFAULT '', "
                "  context TEXT NOT NULL DEFAULT '', "
                "  UNIQUE(file_path, key)"
                ");")) {
        qWarning() << "NstDatabase: Error creating entries table:" << q.lastError().text();
        return false;
    }

    // Indexes
    q.exec("CREATE INDEX IF NOT EXISTS idx_entries_file ON entries(file_path);");
    q.exec("CREATE INDEX IF NOT EXISTS idx_entries_source ON entries(source);");

    // 3. FTS5 Table & Triggers (with fallback if FTS5 not available)
    bool ftsSuccess = q.exec("CREATE VIRTUAL TABLE IF NOT EXISTS entries_fts USING fts5("
                             "  source, text, context, "
                             "  content='entries', content_rowid='id'"
                             ");");
    if (ftsSuccess) {
        q.exec("CREATE TRIGGER IF NOT EXISTS entries_ai AFTER INSERT ON entries BEGIN "
               "  INSERT INTO entries_fts(rowid, source, text, context) VALUES (new.id, new.source, new.text, new.context); "
               "END;");

        q.exec("CREATE TRIGGER IF NOT EXISTS entries_ad AFTER DELETE ON entries BEGIN "
               "  INSERT INTO entries_fts(entries_fts, rowid, source, text, context) VALUES ('delete', old.id, old.source, old.text, old.context); "
               "END;");

        q.exec("CREATE TRIGGER IF NOT EXISTS entries_au AFTER UPDATE ON entries BEGIN "
               "  INSERT INTO entries_fts(entries_fts, rowid, source, text, context) VALUES ('delete', old.id, old.source, old.text, old.context); "
               "  INSERT INTO entries_fts(rowid, source, text, context) VALUES (new.id, new.source, new.text, new.context); "
               "END;");
    } else {
        qWarning() << "NstDatabase: FTS5 not available or failed to initialize:" << q.lastError().text();
    }

    return true;
}

bool NstDatabase::checkAndMigrateSchema()
{
    if (!isOpen()) return false;

    // First ensure tables exist
    if (!createTablesAndTriggers()) return false;

    int currentVer = getSchemaVersion();
    if (currentVer < 1) {
        // Uninitialized schema, set to version 1
        setMeta("schema_version", QString::number(CURRENT_SCHEMA_VERSION));
        currentVer = 1;
    }

    if (currentVer > CURRENT_SCHEMA_VERSION) {
        qWarning() << "NstDatabase: Database schema version" << currentVer 
                   << "is newer than maximum supported version" << CURRENT_SCHEMA_VERSION;
        return false;
    }

    while (currentVer < CURRENT_SCHEMA_VERSION) {
        int nextVer = currentVer + 1;
        if (!runMigration(currentVer, nextVer)) {
            qWarning() << "NstDatabase: Migration from version" << currentVer << "to" << nextVer << "failed.";
            return false;
        }
        currentVer = nextVer;
        setMeta("schema_version", QString::number(currentVer));
    }

    return true;
}

bool NstDatabase::runMigration(int fromVersion, int toVersion)
{
    qDebug() << "NstDatabase: Running migration step from version" << fromVersion << "to" << toVersion;

    /*
     * [NOTE] GUIDANCE FOR FUTURE DEVELOPERS & LLM ASSISTANTS:
     * When bumping CURRENT_SCHEMA_VERSION (e.g. from 1 to 2), add an incremental migration block here:
     *
     * if (fromVersion == 1 && toVersion == 2) {
     *     QSqlQuery q(m_db);
     *     // Example: Add a new column 'status' to entries
     *     if (!q.exec("ALTER TABLE entries ADD COLUMN status TEXT DEFAULT 'draft';")) {
     *         qWarning() << "Migration 1->2 failed:" << q.lastError().text();
     *         return false;
     *     }
     *     return true;
     * }
     */

    Q_UNUSED(fromVersion);
    Q_UNUSED(toVersion);
    return true;
}

QString NstDatabase::getMeta(const QString &key, const QString &defaultValue) const
{
    if (!isOpen()) return defaultValue;
    QSqlQuery q(m_db);
    q.prepare("SELECT value FROM project_meta WHERE key = ?;");
    q.addBindValue(key);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return defaultValue;
}

bool NstDatabase::setMeta(const QString &key, const QString &value)
{
    if (!isOpen()) return false;
    QSqlQuery q(m_db);
    q.prepare("INSERT OR REPLACE INTO project_meta (key, value) VALUES (?, ?);");
    q.addBindValue(key);
    q.addBindValue(value);
    return q.exec();
}

int NstDatabase::getSchemaVersion() const
{
    QString verStr = getMeta("schema_version", "0");
    return verStr.toInt();
}

void NstDatabase::getStats(int &totalEntries, int &translatedEntries) const
{
    totalEntries = 0;
    translatedEntries = 0;
    if (!isOpen()) return;

    QSqlQuery q(m_db);
    if (q.exec("SELECT COUNT(*), SUM(CASE WHEN text != '' THEN 1 ELSE 0 END) FROM entries;")) {
        if (q.next()) {
            totalEntries = q.value(0).toInt();
            translatedEntries = q.value(1).toInt();
        }
    }
}

QStringList NstDatabase::getFileList() const
{
    QStringList files;
    if (!isOpen()) return files;

    QSqlQuery q(m_db);
    if (q.exec("SELECT DISTINCT file_path FROM entries ORDER BY file_path ASC;")) {
        while (q.next()) {
            files.append(q.value(0).toString());
        }
    }
    return files;
}

QJsonArray NstDatabase::getEntriesForFile(const QString &filePath) const
{
    QJsonArray array;
    if (!isOpen()) return array;

    QSqlQuery q(m_db);
    q.prepare("SELECT key, file_path, source, text, context FROM entries WHERE file_path = ? ORDER BY id ASC;");
    q.addBindValue(filePath);

    if (q.exec()) {
        while (q.next()) {
            QJsonObject obj;
            obj["key"] = q.value(0).toString();
            obj["path"] = q.value(1).toString();
            obj["source"] = q.value(2).toString();
            obj["text"] = q.value(3).toString();
            QString ctx = q.value(4).toString();
            if (!ctx.isEmpty()) {
                obj["context"] = ctx;
            }
            array.append(obj);
        }
    }
    return array;
}

QMap<QString, QJsonArray> NstDatabase::getAllDataMap() const
{
    QMap<QString, QJsonArray> dataMap;
    if (!isOpen()) return dataMap;

    QSqlQuery q(m_db);
    if (q.exec("SELECT key, file_path, source, text, context FROM entries ORDER BY file_path ASC, id ASC;")) {
        while (q.next()) {
            QJsonObject obj;
            obj["key"] = q.value(0).toString();
            QString filePath = q.value(1).toString();
            obj["path"] = filePath;
            obj["source"] = q.value(2).toString();
            obj["text"] = q.value(3).toString();
            QString ctx = q.value(4).toString();
            if (!ctx.isEmpty()) {
                obj["context"] = ctx;
            }
            dataMap[filePath].append(obj);
        }
    }
    return dataMap;
}

QJsonArray NstDatabase::getUntranslatedEntries(const QStringList &filePaths) const
{
    QJsonArray array;
    if (!isOpen()) return array;

    QSqlQuery q(m_db);
    QString sql = "SELECT key, file_path, source, text, context FROM entries WHERE text = ''";
    if (!filePaths.isEmpty()) {
        QStringList placeholders;
        for (int i = 0; i < filePaths.size(); ++i) placeholders.append("?");
        sql += QString(" AND file_path IN (%1)").arg(placeholders.join(","));
    }
    sql += " ORDER BY id ASC;";

    q.prepare(sql);
    if (!filePaths.isEmpty()) {
        for (const QString &fp : filePaths) {
            q.addBindValue(fp);
        }
    }

    if (q.exec()) {
        while (q.next()) {
            QJsonObject obj;
            obj["key"] = q.value(0).toString();
            obj["path"] = q.value(1).toString();
            obj["source"] = q.value(2).toString();
            obj["text"] = q.value(3).toString();
            QString ctx = q.value(4).toString();
            if (!ctx.isEmpty()) {
                obj["context"] = ctx;
            }
            array.append(obj);
        }
    }
    return array;
}

bool NstDatabase::insertEntries(const QJsonArray &entries)
{
    if (!isOpen() || entries.isEmpty()) return false;

    m_db.transaction();

    QSqlQuery q(m_db);
    q.prepare("INSERT OR REPLACE INTO entries (file_path, key, source, text, context) VALUES (?, ?, ?, ?, ?);");

    for (const QJsonValue &val : entries) {
        QJsonObject obj = val.toObject();
        QString filePath = obj["path"].toString();
        QString key = obj["key"].toString();
        QString source = obj["source"].toString();
        QString text = obj["text"].toString();
        QString context = obj["context"].toString();

        if (filePath.isEmpty() || key.isEmpty()) continue;

        q.addBindValue(filePath);
        q.addBindValue(key);
        q.addBindValue(source);
        q.addBindValue(text);
        q.addBindValue(context);
        q.exec();
    }

    return m_db.commit();
}

bool NstDatabase::insertOrReplaceAll(const QMap<QString, QJsonArray> &dataMap)
{
    if (!isOpen()) return false;

    m_db.transaction();

    QSqlQuery q(m_db);
    q.prepare("INSERT OR REPLACE INTO entries (file_path, key, source, text, context) VALUES (?, ?, ?, ?, ?);");

    for (auto it = dataMap.begin(); it != dataMap.end(); ++it) {
        const QJsonArray &array = it.value();
        for (const QJsonValue &val : array) {
            QJsonObject obj = val.toObject();
            QString filePath = obj["path"].toString();
            if (filePath.isEmpty()) filePath = it.key();
            QString key = obj["key"].toString();
            QString source = obj["source"].toString();
            QString text = obj["text"].toString();
            QString context = obj["context"].toString();

            if (filePath.isEmpty() || key.isEmpty()) continue;

            q.addBindValue(filePath);
            q.addBindValue(key);
            q.addBindValue(source);
            q.addBindValue(text);
            q.addBindValue(context);
            q.exec();
        }
    }

    return m_db.commit();
}

bool NstDatabase::updateTranslation(const QString &filePath, const QString &source, const QString &translation)
{
    if (!isOpen()) return false;

    QSqlQuery q(m_db);
    if (filePath.isEmpty()) {
        q.prepare("UPDATE entries SET text = ? WHERE source = ?;");
        q.addBindValue(translation);
        q.addBindValue(source);
    } else {
        q.prepare("UPDATE entries SET text = ? WHERE file_path = ? AND source = ?;");
        q.addBindValue(translation);
        q.addBindValue(filePath);
        q.addBindValue(source);
    }

    return q.exec();
}

bool NstDatabase::batchUpdateTranslations(const QList<QJsonObject> &updates)
{
    if (!isOpen() || updates.isEmpty()) return false;

    m_db.transaction();
    QSqlQuery q(m_db);
    q.prepare("UPDATE entries SET text = ? WHERE file_path = ? AND key = ?;");

    for (const QJsonObject &obj : updates) {
        QString filePath = obj["path"].toString();
        QString key = obj["key"].toString();
        QString text = obj["text"].toString();

        q.addBindValue(text);
        q.addBindValue(filePath);
        q.addBindValue(key);
        q.exec();
    }

    return m_db.commit();
}

bool NstDatabase::clearAllData()
{
    if (!isOpen()) return false;
    QSqlQuery q(m_db);
    return q.exec("DELETE FROM entries;");
}

QJsonArray NstDatabase::searchFTS(const QString &searchTerm) const
{
    QJsonArray results;
    if (!isOpen() || searchTerm.trimmed().isEmpty()) return results;

    QSqlQuery q(m_db);
    // Check if FTS5 table exists
    bool ftsExists = q.exec("SELECT 1 FROM entries_fts LIMIT 1;");
    
    if (ftsExists) {
        q.prepare("SELECT e.key, e.file_path, e.source, e.text, e.context "
                  "FROM entries_fts f "
                  "JOIN entries e ON f.rowid = e.id "
                  "WHERE entries_fts MATCH ? "
                  "LIMIT 1000;");
        q.addBindValue(searchTerm);
    } else {
        // Fallback to LIKE query if FTS is unavailable
        q.prepare("SELECT key, file_path, source, text, context "
                  "FROM entries "
                  "WHERE source LIKE ? OR text LIKE ? OR key LIKE ? "
                  "LIMIT 1000;");
        QString pattern = "%" + searchTerm + "%";
        q.addBindValue(pattern);
        q.addBindValue(pattern);
        q.addBindValue(pattern);
    }

    if (q.exec()) {
        while (q.next()) {
            QJsonObject obj;
            obj["key"] = q.value(0).toString();
            obj["path"] = q.value(1).toString();
            obj["source"] = q.value(2).toString();
            obj["text"] = q.value(3).toString();
            QString ctx = q.value(4).toString();
            if (!ctx.isEmpty()) obj["context"] = ctx;
            results.append(obj);
        }
    }
    return results;
}

bool NstDatabase::importFromJson(const QString &jsonFilePath, const QString &projectPath, const QString &engineName)
{
    QFile file(jsonFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "NstDatabase: Cannot open JSON file for import:" << jsonFilePath;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "NstDatabase: Invalid JSON format in:" << jsonFilePath;
        return false;
    }

    QJsonObject root = doc.object();
    QString projPath = projectPath.isEmpty() ? root["projectPath"].toString() : projectPath;
    QString engName = engineName.isEmpty() ? root["engineName"].toString() : engineName;

    setMeta("projectPath", projPath);
    setMeta("engineName", engName);

    QJsonObject dataObj = root["data"].toObject();
    QMap<QString, QJsonArray> dataMap;
    for (auto it = dataObj.begin(); it != dataObj.end(); ++it) {
        if (it.value().isArray()) {
            dataMap.insert(it.key(), it.value().toArray());
        }
    }

    return insertOrReplaceAll(dataMap);
}

bool NstDatabase::exportToJson(const QString &jsonFilePath) const
{
    QJsonObject root;
    root["projectPath"] = getMeta("projectPath", "");
    root["engineName"] = getMeta("engineName", "");

    QJsonObject dataObj;
    QMap<QString, QJsonArray> dataMap = getAllDataMap();
    for (auto it = dataMap.begin(); it != dataMap.end(); ++it) {
        dataObj.insert(it.key(), it.value());
    }
    root["data"] = dataObj;

    QFile file(jsonFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "NstDatabase: Cannot open file for JSON export:" << jsonFilePath;
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
    return true;
}
