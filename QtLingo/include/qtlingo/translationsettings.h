#ifndef TRANSLATIONSETTINGS_H
#define TRANSLATIONSETTINGS_H

#include <QString>
#include <QSettings>
#include <QVariantMap>

struct TranslationSettings {
    // Google Translate Settings
    QString googleApiKey = "";
    bool googleApiEnabled = false; // "googleApi" key in QSettings

    // LLM Settings
    QString llmProvider = "groq";
    QString llmApiKey = "";
    QString llmModel = "llama-3.3-70b-versatile";
    QString llmBaseUrl = "";

    // General Settings
    QString sourceLanguage = "auto";
    QString targetLanguage = "th";
    int translationMode = 0; // 0 for default/batch
    bool deployBackupEnabled = true;

    // Glossary / Terminology
    QMap<QString, QString> glossary;

    // MCP Settings (External Client — NST connects to external MCP servers)
    bool mcpEnabled = false;
    QString mcpServerName = "filesystem";
    QString mcpServerCommand = "npx";
    QString mcpServerArgs = "";

    // Embedded MCP Server Settings (Built-in server — AI agents connect to NST)
    bool embeddedMcpEnabled = true;    // Auto-start when TUI opens
    QString embeddedMcpSocketPath = ""; // Empty = auto-detect default path

    // Load settings from persistent storage (QSettings)
    void load() {
        QSettings settings;
        
        // Google Translate
        googleApiKey = settings.value(QStringLiteral("googleApiKey")).toString();
        // Fallback to "apiKey" if GUI saved it as such, but use "googleApiKey" going forward
        if (googleApiKey.isEmpty()) {
            googleApiKey = settings.value(QStringLiteral("apiKey")).toString();
        }
        googleApiEnabled = settings.value(QStringLiteral("googleApi"), false).toBool();

        // LLM settings
        llmProvider = settings.value(QStringLiteral("llmProvider"), QStringLiteral("groq")).toString();
        llmApiKey = settings.value(QStringLiteral("llmApiKey")).toString();
        llmModel = settings.value(QStringLiteral("llmModel"), QStringLiteral("llama-3.3-70b-versatile")).toString();
        llmBaseUrl = settings.value(QStringLiteral("llmBaseUrl")).toString();

        // General
        sourceLanguage = settings.value(QStringLiteral("sourceLanguage"), QStringLiteral("auto")).toString();
        targetLanguage = settings.value(QStringLiteral("targetLanguage"), QStringLiteral("th")).toString();
        translationMode = settings.value(QStringLiteral("translationMode"), 0).toInt();
        deployBackupEnabled = settings.value(QStringLiteral("deployBackupEnabled"), true).toBool();

        // Glossary
        glossary.clear();
        QVariantMap glossaryMap = settings.value(QStringLiteral("glossary")).toMap();
        for (auto it = glossaryMap.begin(); it != glossaryMap.end(); ++it) {
            glossary[it.key()] = it.value().toString();
        }

        // MCP Settings (External Client)
        mcpEnabled = settings.value(QStringLiteral("mcpEnabled"), false).toBool();
        mcpServerName = settings.value(QStringLiteral("mcpServerName"), QStringLiteral("filesystem")).toString();
        mcpServerCommand = settings.value(QStringLiteral("mcpServerCommand"), QStringLiteral("npx")).toString();
        mcpServerArgs = settings.value(QStringLiteral("mcpServerArgs"), QStringLiteral("-y @modelcontextprotocol/server-filesystem .")).toString();

        // Embedded MCP Server Settings
        embeddedMcpEnabled = settings.value(QStringLiteral("embeddedMcpEnabled"), true).toBool();
        embeddedMcpSocketPath = settings.value(QStringLiteral("embeddedMcpSocketPath")).toString();
    }

    // Save settings to persistent storage (QSettings)
    void save() const {
        QSettings settings;

        // Google Translate
        settings.setValue(QStringLiteral("googleApiKey"), googleApiKey);
        settings.setValue(QStringLiteral("googleApi"), googleApiEnabled);

        // LLM settings
        settings.setValue(QStringLiteral("llmProvider"), llmProvider);
        settings.setValue(QStringLiteral("llmApiKey"), llmApiKey);
        settings.setValue(QStringLiteral("llmModel"), llmModel);
        settings.setValue(QStringLiteral("llmBaseUrl"), llmBaseUrl);

        // General
        settings.setValue(QStringLiteral("sourceLanguage"), sourceLanguage);
        settings.setValue(QStringLiteral("targetLanguage"), targetLanguage);
        settings.setValue(QStringLiteral("translationMode"), translationMode);
        settings.setValue(QStringLiteral("deployBackupEnabled"), deployBackupEnabled);

        // Glossary
        QVariantMap glossaryMap;
        for (auto it = glossary.begin(); it != glossary.end(); ++it) {
            glossaryMap[it.key()] = it.value();
        }
        settings.setValue(QStringLiteral("glossary"), glossaryMap);

        // MCP Settings (External Client)
        settings.setValue(QStringLiteral("mcpEnabled"), mcpEnabled);
        settings.setValue(QStringLiteral("mcpServerName"), mcpServerName);
        settings.setValue(QStringLiteral("mcpServerCommand"), mcpServerCommand);
        settings.setValue(QStringLiteral("mcpServerArgs"), mcpServerArgs);

        // Embedded MCP Server Settings
        settings.setValue(QStringLiteral("embeddedMcpEnabled"), embeddedMcpEnabled);
        settings.setValue(QStringLiteral("embeddedMcpSocketPath"), embeddedMcpSocketPath);
    }

    // Convert to QVariantMap to configure TranslationCore & plugins
    QVariantMap toVariantMap() const {
        QVariantMap map;
        map[QStringLiteral("googleApiKey")] = googleApiKey;
        map[QStringLiteral("googleApi")] = googleApiEnabled;
        map[QStringLiteral("llmProvider")] = llmProvider;
        map[QStringLiteral("llmApiKey")] = llmApiKey;
        map[QStringLiteral("llmModel")] = llmModel;
        map[QStringLiteral("llmBaseUrl")] = llmBaseUrl;
        map[QStringLiteral("sourceLanguage")] = sourceLanguage;
        map[QStringLiteral("targetLanguage")] = targetLanguage;
        map[QStringLiteral("mcpEnabled")] = mcpEnabled;
        map[QStringLiteral("mcpServerName")] = mcpServerName;
        map[QStringLiteral("mcpServerCommand")] = mcpServerCommand;
        map[QStringLiteral("mcpServerArgs")] = mcpServerArgs;
        map[QStringLiteral("embeddedMcpEnabled")] = embeddedMcpEnabled;
        map[QStringLiteral("embeddedMcpSocketPath")] = embeddedMcpSocketPath;
        return map;
    }
};

#endif // TRANSLATIONSETTINGS_H
