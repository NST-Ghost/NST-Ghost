#include "rpgm_injection_exporter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QTextStream>
#include <QCoreApplication>
#include <QDebug>
#include <iostream>

RpgmInjectionExporter::RpgmInjectionExporter(QObject *parent)
    : QObject(parent)
{
}

bool RpgmInjectionExporter::deploy(const QString &gamePath,
                                    const QMap<QString, QJsonArray> &data,
                                    bool onlyTranslated,
                                    const QString &languageName)
{
    std::cerr << "[NST Injection] Starting injection deploy to: "
              << gamePath.toStdString() << std::endl;

    // 1. Create translation folder
    emit progressUpdated(10, "Creating translation folder...");
    if (!createTranslationFolder(gamePath)) {
        return false;
    }

    QString translationDir = QDir(gamePath).filePath("nst_translations");

    // 2. Write config file
    emit progressUpdated(20, "Writing config.json...");
    if (!writeConfigFile(translationDir, languageName)) {
        return false;
    }

    // 3. Export each file's translations
    int totalFiles = data.size();
    int processed = 0;

    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        const QString &filePath = it.key();
        const QJsonArray &entries = it.value();

        QString baseName = fileBaseName(filePath);
        if (baseName.isEmpty()) {
            processed++;
            continue;
        }

        int pct = 20 + (processed * 60 / qMax(totalFiles, 1));
        emit progressUpdated(pct, QString("Exporting %1.json...").arg(baseName));

        writeTranslationFile(translationDir, baseName, entries, onlyTranslated);
        processed++;
    }

    // 4. Copy JS plugin
    emit progressUpdated(85, "Copying plugin...");
    if (!copyPluginFile(gamePath)) {
        emit errorOccurred("Failed to copy NST_TranslationLayer.js plugin file.");
        // Non-fatal: user can copy manually
    }

    // 5. Auto-patch plugins.js so end user doesn't need to edit anything
    emit progressUpdated(95, "Patching plugins.js...");
    patchPluginsJs(gamePath);

    emit progressUpdated(100, "Injection deploy complete.");
    std::cerr << "[NST Injection] Deploy complete. "
              << processed << " files exported." << std::endl;
    return true;
}

bool RpgmInjectionExporter::createTranslationFolder(const QString &gamePath)
{
    QDir gameDir(gamePath);
    QString translationPath = gameDir.filePath("nst_translations");

    if (!QDir(translationPath).exists()) {
        if (!QDir().mkpath(translationPath)) {
            emit errorOccurred("Failed to create directory: " + translationPath);
            return false;
        }
    }

    return true;
}

bool RpgmInjectionExporter::writeConfigFile(const QString &translationDir,
                                             const QString &languageName)
{
    QString configPath = QDir(translationDir).filePath("config.json");
    QFile file(configPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        emit errorOccurred("Failed to write config file: " + configPath);
        return false;
    }

    // Build the config.json object that drives the runtime translation layer.
    // This mirrors the schema in Injection/rpgm_ts/src/utils/json-loader.ts.
    QJsonObject config;
    config["__sourceLocale__"] = QStringLiteral("ja");
    config["__languageName__"] = languageName;

    // --- Text fields translated inside game data objects ---
    const QJsonArray textFields {
        QStringLiteral("name"), QStringLiteral("description"),
        QStringLiteral("displayName"), QStringLiteral("nickname"),
        QStringLiteral("profile"), QStringLiteral("message1"),
        QStringLiteral("message2"), QStringLiteral("message3"),
        QStringLiteral("message4"), QStringLiteral("gameTitle"),
        QStringLiteral("terms"), QStringLiteral("messages"),
    };
    config["__textFields__"] = textFields;

    // --- Event command codes whose parameter indices hold translatable text ---
    QJsonObject textCommands;
    textCommands["101"] = QJsonArray{4};
    textCommands["102"] = QJsonArray{0};
    textCommands["320"] = QJsonArray{1};
    textCommands["324"] = QJsonArray{1};
    textCommands["325"] = QJsonArray{1};
    textCommands["402"] = QJsonArray{1};
    textCommands["405"] = QJsonArray{0};
    config["__textCommands__"] = textCommands;

    // --- Control character patterns (extracted before translation) ---
    // These preserve RPG Maker escape codes (\\V[n], \\C[n], \\I[n], ...) and
    // common plugin escape codes so that they are not corrupted by translation.
    const QJsonArray controlCharPatterns {
        QStringLiteral("\\\\[VNP]\\[\\d+\\]"),
        QStringLiteral("\\\\I\\[\\d+\\]"),
        QStringLiteral("\\\\C\\[\\d+\\]"),
        QStringLiteral("\\\\G"),
        QStringLiteral("\\\\[{}]"),
        QStringLiteral("\\\\\\$"),
        QStringLiteral("\\\\[.|]"),
        QStringLiteral("\\\\!"),
        QStringLiteral("\\\\[><]"),
        QStringLiteral("\\\\\\^"),
        QStringLiteral("\\\\\\\\"),
        QStringLiteral("\\\\FS\\[\\d+\\]"),
        QStringLiteral("\\\\P[XY]\\[-?\\d+\\]"),
        QStringLiteral("\\\\[OT]C\\[\\d+\\]"),
        QStringLiteral("\\\\(?:MSGCORE|MSGSND)\\[[^\\]]*\\]"),
    };
    config["__controlCharPatterns__"] = controlCharPatterns;

    // --- Ignore patterns (skipped, never translated) ---
    const QJsonArray ignorePatterns {
        QStringLiteral("^.$"),
        QStringLiteral("^\\s*$"),
        QStringLiteral("^[\\d\\s.,\\-+%$/\\\\:;()\\[\\]{}=*#@!?<>~`'\"^&|_]+$"),
        QStringLiteral("^\\d+$"),
        QStringLiteral("\\.(png|jpe?g|gif|bmp|webp|svg|ico)$"),
        QStringLiteral("\\.(ogg|m4a|mp3|wav|flac|aac|wma)$"),
        QStringLiteral("^https?://"),
        QStringLiteral("^data:"),
    };
    config["__ignorePatterns__"] = ignorePatterns;

    // --- Font configuration ---
    QJsonObject fontConfig;
    fontConfig["fontName"] = QStringLiteral("NotoSans");
    fontConfig["fontUrl"] = QStringLiteral("fonts/NotoSans-Regular.woff2");
    fontConfig["offsetSize"] = 0;
    fontConfig["maxSizeOffset"] = 0;
    config["__fontConfig__"] = fontConfig;

    // --- Custom hooks (config-driven; equivalent to the legacy hardcoded hooks) ---
    config["__customHooks__"] = defaultHooks();

    QJsonDocument doc(config);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    std::cerr << "[NST Injection] Wrote config.json (" << languageName.toStdString() << ")\n";
    return true;
}

// Build the default custom-hooks array. These replicate the behaviour of the
// legacy hardcoded prototype patching in NST_TranslationLayer.js. Keeping them
// in config (rather than baked into the JS) lets users enable/disable or add
// new hooks for third-party plugins without rebuilding the plugin.
QJsonArray RpgmInjectionExporter::defaultHooks() const
{
    QJsonArray hooks;

    auto add = [&](const char *cls, const char *method, const char *type,
                   int paramIndex = 0, bool enabled = true,
                   const QJsonArray &nestedIndex = {},
                   int minParamLength = 0,
                   const char *title = nullptr, const char *desc = nullptr) {
        QJsonObject h;
        h["class"] = QString::fromLatin1(cls);
        h["method"] = QString::fromLatin1(method);
        h["type"] = QString::fromLatin1(type);
        h["paramIndex"] = paramIndex;
        h["enabled"] = enabled;
        if (!nestedIndex.isEmpty()) h["nestedIndex"] = nestedIndex;
        if (minParamLength > 0) h["minParamLength"] = minParamLength;
        if (title) h["title"] = QString::fromLatin1(title);
        if (desc)  h["desc"]  = QString::fromLatin1(desc);
        hooks.append(h);
    };

    // --- Core text pipeline hooks (mirror legacy _o1.._o12) ---
    add("Window_Base", "convertEscapeCharacters", "tr0", 0, true,
        {}, 0, "Main text pipeline", "Translate text through the escape-character converter.");
    add("Game_Message", "add", "tr0", 0, true,
        {}, 0, "Dialogue message text", "Translate dialogue message text as it is added.");
    add("Game_Message", "setSpeakerName", "tr0", 0, true,
        {}, 0, "Speaker name", "Translate the speaker name shown above dialogue.");
    add("Game_Interpreter", "command101", "trNested", 0, true,
        {4}, 5, "Show Text command", "Translate event command 101 (Show Text) text parameter.");
    add("Game_Interpreter", "command102", "trNestedArray", 0, true,
        {0}, 0, "Show Choices command", "Translate event command 102 (Show Choices) choice text.");
    add("DataManager", "onLoad", "trData", 0, true,
        {}, 0, "Data load translation", "Translate text content in game data objects on load.");
    add("Window_Base", "drawText", "tr0", 0, true,
        {}, 0, "Window text draw", "Translate text rendered by Window_Base.drawText.");
    add("Window_Base", "drawTextEx", "tr0", 0, true,
        {}, 0, "Extended text draw", "Translate text rendered by Window_Base.drawTextEx.");
    add("Window_Base", "textWidth", "tr0", 0, true,
        {}, 0, "Text width calc", "Translate text during width measurement for correct layout.");
    add("Window_Base", "createTextState", "tr0", 0, true,
        {}, 0, "Text state creation", "Translate text in Window_Base.createTextState.");
    add("Bitmap", "drawText", "tr0", 0, true,
        {}, 0, "Bitmap text draw", "Translate text rendered by Bitmap.drawText.");
    add("Bitmap", "measureTextWidth", "tr0", 0, true,
        {}, 0, "Bitmap text measure", "Translate text during Bitmap.measureTextWidth.");

    // --- Third-party plugin hooks (disabled by default; enable per game) ---
    add("Window_NameBox", "refresh", "trThisAfter", 0, false,
        {}, 0, "Speaker name box", "YEP_MessageCore/VisuMZ plugin name box.");
    add("Window_GabWindow", "refresh", "trThisAfter", 0, false,
        {}, 0, "Gab window", "YEP_GabWindow/VisuMZ_2_GabWindow plugin.");
    add("Window_QuestData", "refresh", "trThisAfter", 0, false,
        {}, 0, "Quest detail window", "YEP_QuestJournal plugin quest details.");

    return hooks;
}

bool RpgmInjectionExporter::writeTranslationFile(const QString &translationDir,
                                                   const QString &baseName,
                                                   const QJsonArray &entries,
                                                   bool onlyTranslated)
{
    // Build a { "source": "translated" } object from the entries.
    QJsonObject translations;

    int written = 0;
    for (const QJsonValue &val : entries) {
        QJsonObject obj = val.toObject();
        QString source = obj["source"].toString();
        QString translated = obj["text"].toString();

        if (source.isEmpty()) continue;
        if (onlyTranslated && translated.isEmpty()) continue;
        if (translated.isEmpty()) continue; // Skip empty translations always.
        if (translated == source) continue;  // Skip no-op translations.

        // Regex entries: source is "/pattern/flags", emit as part of __regex__.
        // Kept here for forward compatibility; the runtime layer reads __regex__.
        static const QRegularExpression rxEntry(QStringLiteral("^/(.+)/([gimsuy]*)$"));
        auto rxMatch = rxEntry.match(source);
        if (rxMatch.hasMatch()) {
            // Defer regex entries; collected below.
            continue;
        }

        translations[source] = translated;
        written++;
    }

    if (written == 0) {
        return true; // Nothing to write; skip this file.
    }

    QString filePath = QDir(translationDir).filePath(baseName + ".json");
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        emit errorOccurred("Failed to write translation file: " + filePath);
        return false;
    }

    QJsonDocument doc(translations);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    std::cerr << "[NST Injection] Wrote " << written
              << " entries to " << baseName.toStdString() << ".json\n";
    return true;
}

bool RpgmInjectionExporter::copyPluginFile(const QString &gamePath)
{
    // Determine js/plugins/ path (MV uses www/js/plugins, MZ uses js/plugins)
    QDir gameDir(gamePath);
    QString pluginDir;

    if (gameDir.exists("www/js/plugins")) {
        pluginDir = gameDir.filePath("www/js/plugins");
    } else if (gameDir.exists("js/plugins")) {
        pluginDir = gameDir.filePath("js/plugins");
    } else {
        // Try to create js/plugins
        pluginDir = gameDir.filePath("js/plugins");
        if (!QDir().mkpath(pluginDir)) {
            return false;
        }
    }

    QString destPath = QDir(pluginDir).filePath("NST_TranslationLayer.js");
    QString srcPath = pluginSourcePath();

    // Remove existing copy
    if (QFile::exists(destPath)) {
        QFile::remove(destPath);
    }

    if (QFile::exists(srcPath)) {
        return QFile::copy(srcPath, destPath);
    }

    // If source not found in expected location, try relative to app
    qWarning() << "[NST Injection] Plugin source not found at:" << srcPath;
    return false;
}

void RpgmInjectionExporter::patchPluginsJs(const QString &gamePath)
{
    QDir gameDir(gamePath);

    // Find plugins.js (MV: www/js/plugins.js, MZ: js/plugins.js)
    QString pluginsJsPath;
    if (gameDir.exists("www/js/plugins.js"))
        pluginsJsPath = gameDir.filePath("www/js/plugins.js");
    else if (gameDir.exists("js/plugins.js"))
        pluginsJsPath = gameDir.filePath("js/plugins.js");
    else
        return; // No plugins.js found, skip

    QFile file(pluginsJsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // Check if already patched
    if (content.contains("NST_TranslationLayer"))
        return;

    // Find the closing bracket of the $plugins array: ];
    // Insert our entry before it
    static const QString entry =
        QStringLiteral("{\"name\":\"NST_TranslationLayer\",\"status\":true,"
                       "\"description\":\"NST Translation Layer\",\"parameters\":{}}");

    int closeBracket = content.lastIndexOf(QStringLiteral("];"));
    if (closeBracket < 0)
        return;

    // Determine if we need a comma (check if there are existing entries)
    QString before = content.left(closeBracket).trimmed();
    bool needComma = !before.isEmpty() && !before.endsWith('[');

    QString insertion = (needComma ? QStringLiteral(",\n") : QStringLiteral("\n"))
                        + entry + QStringLiteral("\n");
    content.insert(closeBracket, insertion);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    file.write(content.toUtf8());
    file.close();

    std::cerr << "[NST Injection] Patched plugins.js" << std::endl;
}

QString RpgmInjectionExporter::fileBaseName(const QString &filePath) const
{
    QFileInfo info(filePath);
    QString name = info.completeBaseName();
    return name;
}

QString RpgmInjectionExporter::pluginSourcePath()
{
    QStringList candidates;
    QString appDir = QCoreApplication::applicationDirPath();

    candidates << appDir + "/Injection/rpgm/NST_TranslationLayer.js"
               << appDir + "/../Injection/rpgm/NST_TranslationLayer.js"
               << appDir + "/../../Injection/rpgm/NST_TranslationLayer.js";

    for (const QString &path : candidates) {
        if (QFile::exists(path)) {
            return QFileInfo(path).absoluteFilePath();
        }
    }

    return candidates.first();
}
