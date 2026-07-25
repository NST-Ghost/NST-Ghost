#include <QtTest/QtTest>
#include "core/engines/rpgm/rpganalyzer.h"
#include <QDir>
#include <QFile>
#include <QDirIterator>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

class TestRpgAnalyzer : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> tempDir;
    QString dataPath;

private slots:
    void initTestCase()
    {
        tempDir = std::make_unique<QTemporaryDir>();
        QVERIFY(tempDir->isValid());
        
        QDir dir(tempDir->path());
        dir.mkdir("data");
        dataPath = dir.filePath("data");
    }

    void testCoverage()
    {
        // Generate mock data for coverage test
        QString mockFileName = "coverage_mock.json";
        QString mockFilePath = dataPath + "/" + mockFileName;
        
        QJsonArray parameters;
        parameters.append("Some text to extract");
        
        QJsonObject command;
        command.insert("code", 401); // Message
        command.insert("indent", 0);
        command.insert("parameters", parameters);
        
        QJsonArray list;
        list.append(command);
        
        QJsonObject event;
        event.insert("id", 1);
        event.insert("list", list);
        
        QJsonArray root;
        root.append(QJsonValue::Null);
        root.append(event);
        
        QJsonDocument doc(root);
        QFile file(mockFilePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(doc.toJson());
        file.close();

        core::engines::rpgm::RpgmAnalyzer analyzer;
        core::AnalyzerOutput output = analyzer.analyze(tempDir->path()); 

        QJsonDocument outDoc = QJsonDocument::fromJson(output.payload);
        QJsonArray strings = outDoc.object()["strings"].toArray();
        
        qDebug() << "==========================================";
        qDebug() << "          EXTRACTION COVERAGE REPORT      ";
        qDebug() << "==========================================";
        qDebug() << "Total Strings Extracted: " << strings.size();
        
        for(const auto& val : strings) {
            QJsonObject obj = val.toObject();
             // Clean newlines for cleaner output
            QString source = obj["source"].toString().replace("\n", "\\n");
            qInfo() << "[EXTRACTED] " << QFileInfo(obj["path"].toString()).fileName() << " | " << obj["key"].toString() << " | " << source;
        }

        qDebug() << "==========================================";
        
        QVERIFY(strings.size() > 0);
    }

    void testShortStrings()
    {
        // 1. Create a mock file with short strings
        QString mockFileName = "short_strings.json";
        QString mockFilePath = dataPath + "/" + mockFileName;
        
        QJsonArray choices;
        choices.append("Yes");
        choices.append("No");
        choices.append("A");
        choices.append("Hi");
        
        QJsonArray parameters;
        parameters.append(choices); // Index 0: Array of choices
        parameters.append(1);       // Index 1: Cancel Type
        
        QJsonObject command;
        command.insert("code", 102); // Show Choices
        command.insert("indent", 0);
        command.insert("parameters", parameters);
        
        QJsonArray list;
        list.append(command);
        
        QJsonObject event;
        event.insert("id", 1);
        event.insert("list", list);
        
        QJsonArray root;
        root.append(QJsonValue::Null);
        root.append(event);
        
        QJsonDocument doc(root);
        QFile file(mockFilePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(doc.toJson());
        file.close();
        
        // 2. Run Analyzer
        core::engines::rpgm::RpgmAnalyzer analyzer;
        core::AnalyzerOutput output = analyzer.analyze(tempDir->path());
        
        QJsonDocument outDoc = QJsonDocument::fromJson(output.payload);
        QJsonArray strings = outDoc.object()["strings"].toArray();
        
        // 3. Verify
        int foundCount = 0;
        QStringList foundTexts;
        for(const QJsonValue &val : strings) {
            QJsonObject obj = val.toObject();
            if (obj["path"].toString() == mockFilePath) {
                QString text = obj["source"].toString();
                foundTexts << text;
                if (text == "Yes" || text == "No" || text == "A" || text == "Hi") {
                    foundCount++;
                }
            }
        }
        
        if (foundCount != 4) {
            qDebug() << "Found texts in short_strings.json:" << foundTexts;
        }
        QCOMPARE(foundCount, 4);
    }

    void testSave()
    {
        // 1. Setup a simple mock file
        QString mockFileName = "save_test.json";
        QString mockFilePath = dataPath + "/" + mockFileName;
        
        QJsonArray parameters;
        parameters.append("Original Text");
        
        QJsonObject command;
        command.insert("code", 401);
        command.insert("indent", 0);
        command.insert("parameters", parameters);
        
        QJsonArray list;
        list.append(command);
        
        QJsonObject event;
        event.insert("id", 1);
        event.insert("list", list);
        
        QJsonArray root;
        root.append(QJsonValue::Null); // Index 0 is null
        root.append(event);            // Index 1 is the event
        
        QJsonDocument doc(root);
        QFile file(mockFilePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(doc.toJson());
        file.close();
        
        // 2. Prepare translation payload
        // We know the key path will be "1.list[0].parameters[0]" based on analysis
        QJsonObject entry;
        entry.insert("source", "Original Text");
        entry.insert("path", mockFilePath);
        entry.insert("key", "1.list[0].parameters[0]");
        entry.insert("text", "Translated Text");
        
        QJsonArray texts;
        texts.append(entry);
        
        // 3. Call Save
        core::engines::rpgm::RpgmAnalyzer analyzer;
        bool success = analyzer.save(tempDir->path(), texts);
        QVERIFY(success);
        
        // 4. Verify content
        QFile checkFile(mockFilePath);
        QVERIFY(checkFile.open(QIODevice::ReadOnly));
        QJsonDocument checkDoc = QJsonDocument::fromJson(checkFile.readAll());
        checkFile.close();
        
        QJsonArray checkRoot = checkDoc.array();
        QJsonObject checkEvent = checkRoot[1].toObject();
        QJsonArray checkList = checkEvent["list"].toArray();
        QJsonValue checkCommandVal = checkList[0];
        
        // FAIL CONDITION: If the bug exists, checkCommandVal will be a String.
        // SUCCESS CONDITION: checkCommandVal must be an Object.
        
        if (checkCommandVal.isString()) {
             qCritical() << "BUG DETECTED: list[0] became a string: " << checkCommandVal.toString();
        } else if (checkCommandVal.isObject()) {
             qDebug() << "SUCCESS: list[0] is still an object.";
             QJsonObject checkCommand = checkCommandVal.toObject();
             QJsonArray checkParams = checkCommand["parameters"].toArray();
             qDebug() << "Param[0]: " << checkParams[0].toString();
             QCOMPARE(checkParams[0].toString(), QString("Translated Text"));
        } else {
             qCritical() << "Unknown type at list[0]";
        }
        
        QVERIFY2(checkCommandVal.isObject(), "Structure corrupted: Command object replaced by string!");
    }
};

QTEST_MAIN(TestRpgAnalyzer)
#include "test_rpg_analyzer.moc"
