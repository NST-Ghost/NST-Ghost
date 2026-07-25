#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QElapsedTimer>
#include <lua.hpp>

#include "../src/db/nstdatabase.h"
#include "../src/managers/projectdatamanager.h"

// Global Qt Environment for tests requiring QCoreApplication
class NstTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        int argc = 1;
        static char appName[] = "NstTest";
        static char* argv[] = { appName, nullptr };
        if (!QCoreApplication::instance()) {
            app = new QCoreApplication(argc, argv);
        }
    }
    void TearDown() override {
        delete app;
        app = nullptr;
    }
private:
    QCoreApplication* app = nullptr;
};

class NstDatabaseTest : public ::testing::Test {
protected:
    QString tempDbPath;
    QString tempJsonPath;
    QString importDbPath;

    void SetUp() override {
        tempDbPath = QDir::tempPath() + "/nst_test_db.sqlite";
        tempJsonPath = QDir::tempPath() + "/nst_test_legacy.nst";
        importDbPath = QDir::tempPath() + "/nst_test_imported.sqlite";

        QFile::remove(tempDbPath);
        QFile::remove(tempJsonPath);
        QFile::remove(tempJsonPath + ".json.bak");
        QFile::remove(importDbPath);
    }

    void TearDown() override {
        QFile::remove(tempDbPath);
        QFile::remove(tempJsonPath);
        QFile::remove(tempJsonPath + ".json.bak");
        QFile::remove(importDbPath);
    }
};

TEST_F(NstDatabaseTest, CreateDatabase) {
    NstDatabase db;
    ASSERT_TRUE(db.create(tempDbPath));
    EXPECT_TRUE(db.isOpen());
    EXPECT_TRUE(NstDatabase::isSQLiteFile(tempDbPath));
    EXPECT_EQ(db.getSchemaVersion(), NstDatabase::CURRENT_SCHEMA_VERSION);
}

TEST_F(NstDatabaseTest, MetadataOperations) {
    NstDatabase db;
    ASSERT_TRUE(db.create(tempDbPath));
    db.setMeta("projectPath", "/home/user/games/MyGame");
    db.setMeta("engineName", "RPGM");
    EXPECT_EQ(db.getMeta("projectPath"), QString("/home/user/games/MyGame"));
    EXPECT_EQ(db.getMeta("engineName"), QString("RPGM"));
}

TEST_F(NstDatabaseTest, DataInsertionAndStats) {
    NstDatabase db;
    ASSERT_TRUE(db.create(tempDbPath));

    QJsonArray entries;
    QJsonObject e1;
    e1["key"] = "1.name";
    e1["path"] = "data/Actors.json";
    e1["source"] = "ハイド";
    e1["text"] = "Hyde";

    QJsonObject e2;
    e2["key"] = "2.name";
    e2["path"] = "data/Actors.json";
    e2["source"] = "ジルコ";
    e2["text"] = "";

    QJsonObject e3;
    e3["key"] = "1.text";
    e3["path"] = "data/System.json";
    e3["source"] = "ゲームオーバー";
    e3["text"] = "Game Over";

    entries.append(e1);
    entries.append(e2);
    entries.append(e3);

    ASSERT_TRUE(db.insertEntries(entries));

    QStringList files = db.getFileList();
    EXPECT_EQ(files.size(), 2);
    EXPECT_TRUE(files.contains("data/Actors.json"));
    EXPECT_TRUE(files.contains("data/System.json"));

    int total = 0, translated = 0;
    db.getStats(total, translated);
    EXPECT_EQ(total, 3);
    EXPECT_EQ(translated, 2);

    QJsonArray actorEntries = db.getEntriesForFile("data/Actors.json");
    EXPECT_EQ(actorEntries.size(), 2);
}

TEST_F(NstDatabaseTest, SingleUpdateAndSearch) {
    NstDatabase db;
    ASSERT_TRUE(db.create(tempDbPath));

    QJsonArray entries;
    QJsonObject e1;
    e1["key"] = "2.name";
    e1["path"] = "data/Actors.json";
    e1["source"] = "ジルコ";
    e1["text"] = "";

    QJsonObject e2;
    e2["key"] = "1.text";
    e2["path"] = "data/System.json";
    e2["source"] = "ゲームオーバー";
    e2["text"] = "Game Over";

    entries.append(e1);
    entries.append(e2);
    ASSERT_TRUE(db.insertEntries(entries));

    EXPECT_TRUE(db.updateTranslation("data/Actors.json", "ジルコ", "Jirko"));
    
    int total = 0, translated = 0;
    db.getStats(total, translated);
    EXPECT_EQ(translated, 2);

    QJsonArray searchResults = db.searchFTS("Game Over");
    ASSERT_GE(searchResults.size(), 1);
    EXPECT_EQ(searchResults.at(0).toObject()["source"].toString(), QString("ゲームオーバー"));
}

TEST_F(NstDatabaseTest, JsonImportExport) {
    NstDatabase db;
    ASSERT_TRUE(db.create(tempDbPath));
    db.setMeta("engineName", "RPGM");

    QJsonArray entries;
    QJsonObject e1;
    e1["key"] = "1.text";
    e1["path"] = "data/System.json";
    e1["source"] = "ゲームオーバー";
    e1["text"] = "Game Over";
    entries.append(e1);
    ASSERT_TRUE(db.insertEntries(entries));

    ASSERT_TRUE(db.exportToJson(tempJsonPath));
    EXPECT_TRUE(QFile::exists(tempJsonPath));
    db.close();

    NstDatabase importDb;
    ASSERT_TRUE(importDb.create(importDbPath));
    ASSERT_TRUE(importDb.importFromJson(tempJsonPath));
    EXPECT_EQ(importDb.getMeta("engineName"), QString("RPGM"));
    EXPECT_EQ(importDb.getFileList().size(), 1);
    importDb.close();
}

TEST_F(NstDatabaseTest, ProjectDataManagerIntegration) {
    NstDatabase db;
    ASSERT_TRUE(db.create(tempDbPath));
    db.setMeta("engineName", "RPGM");
    db.setMeta("projectPath", "/home/user/games/MyGame");

    QJsonArray entries;
    QJsonObject e1;
    e1["key"] = "1.text";
    e1["path"] = "data/System.json";
    e1["source"] = "ゲームオーバー";
    e1["text"] = "Game Over";
    entries.append(e1);
    ASSERT_TRUE(db.insertEntries(entries));
    ASSERT_TRUE(db.exportToJson(tempJsonPath));
    db.close();

    ProjectDataManager manager;
    ASSERT_TRUE(manager.loadTranslationWorkspace(tempJsonPath));
    EXPECT_EQ(manager.getEngineName(), QString("RPGM"));
    EXPECT_EQ(manager.getProjectPath(), QString("/home/user/games/MyGame"));

    manager.updateTranslation("ゲームオーバー", "Game Over (Updated)", "data/System.json");
    EXPECT_EQ(manager.database().getEntriesForFile("data/System.json").at(0).toObject()["text"].toString(), QString("Game Over (Updated)"));
}

TEST_F(NstDatabaseTest, FastProjectLoadingWithoutFullMemoryDump) {
    // Test that opening an SQLite workspace is instant and performs lazy cache initialization
    NstDatabase db;
    ASSERT_TRUE(db.create(tempDbPath));
    db.setMeta("engineName", "NodeNetwork_Test");
    db.setMeta("projectPath", "/home/user/games/FastTestGame");

    // Insert 500 entries across 5 files
    QJsonArray entries;
    for (int fileIdx = 1; fileIdx <= 5; ++fileIdx) {
        QString file = QString("data/Map%1.json").arg(fileIdx, 3, 10, QChar('0'));
        for (int i = 1; i <= 100; ++i) {
            QJsonObject entry;
            entry["key"] = QString("%1.text").arg(i);
            entry["path"] = file;
            entry["source"] = QString("Text entry %1 in file %2").arg(i).arg(fileIdx);
            entry["text"] = QString("Translated entry %1").arg(i);
            entries.append(entry);
        }
    }
    ASSERT_TRUE(db.insertEntries(entries));
    db.close();

    // Measure workspace loading time
    QElapsedTimer timer;
    timer.start();

    ProjectDataManager manager;
    ASSERT_TRUE(manager.loadTranslationWorkspace(tempDbPath));
    qint64 loadTimeMs = timer.elapsed();

    // Opening SQLite project should take less than 100ms
    EXPECT_LT(loadTimeMs, 100);
    EXPECT_EQ(manager.getEngineName(), QString("NodeNetwork_Test"));
    EXPECT_EQ(manager.getProjectPath(), QString("/home/user/games/FastTestGame"));
    EXPECT_EQ(manager.database().getFileList().size(), 5);

    // Verify lazy loading of cache when accessed
    QMap<QString, QJsonArray> &cachedData = manager.getLoadedGameProjectData();
    EXPECT_EQ(cachedData.size(), 5);
    EXPECT_EQ(cachedData.value("data/Map001.json").size(), 100);
}

TEST_F(NstDatabaseTest, NodeNetworkLuaScriptModelsHook) {
    // Test that Lua plugin script exports valid model list
    QString scriptPath = "scripts/lua/nodenetwork_translate.lua";
    if (!QFile::exists(scriptPath)) {
        scriptPath = "../scripts/lua/nodenetwork_translate.lua";
    }
    ASSERT_TRUE(QFile::exists(scriptPath));

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    ASSERT_EQ(luaL_dofile(L, scriptPath.toStdString().c_str()), LUA_OK);

    lua_getglobal(L, "on_get_models");
    ASSERT_TRUE(lua_isfunction(L, -1));

    ASSERT_EQ(lua_pcall(L, 0, 1, 0), LUA_OK);
    ASSERT_TRUE(lua_istable(L, -1));

    int count = lua_rawlen(L, -1);
    EXPECT_GE(count, 20); // Should contain at least 20 models including gemini-3.6-flash

    bool foundGemini36 = false;
    for (int i = 1; i <= count; ++i) {
        lua_rawgeti(L, -1, i);
        if (lua_isstring(L, -1)) {
            QString modelStr = QString::fromUtf8(lua_tostring(L, -1));
            if (modelStr == "gemini-3.6-flash") {
                foundGemini36 = true;
            }
        }
        lua_pop(L, 1);
    }
    EXPECT_TRUE(foundGemini36);
    lua_close(L);
}

TEST_F(NstDatabaseTest, RpgmFixtureLoad) {
    QString fixturePath = QDir::currentPath() + "/tests/fixtures/rpgm/Actors.json";
    if (!QFile::exists(fixturePath)) {
        fixturePath = QDir(QCoreApplication::applicationDirPath()).filePath("../tests/fixtures/rpgm/Actors.json");
    }
    if (!QFile::exists(fixturePath)) {
        fixturePath = "../tests/fixtures/rpgm/Actors.json";
    }
    ASSERT_TRUE(QFile::exists(fixturePath)) << "Fixture file not found at " << fixturePath.toStdString();

    QFile file(fixturePath);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    ASSERT_TRUE(doc.isArray());

    QJsonArray array = doc.array();
    ASSERT_GE(array.size(), 2);
    QJsonObject actor = array.at(1).toObject();
    EXPECT_EQ(actor["name"].toString(), QString("ハイド"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new NstTestEnvironment);
    return RUN_ALL_TESTS();
}
