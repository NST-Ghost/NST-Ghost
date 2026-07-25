#include <QCoreApplication>
#include <QDebug>
#include <QSettings>
#include <QTimer>
#include <QFile>
#include <iostream>
#include "plugins/LuaTranslationPlugin/luatranslationservice.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("NST");
    QCoreApplication::setApplicationName("NST");
    QSettings::setDefaultFormat(QSettings::IniFormat);

    std::cout << "\n========================================================\n";
    std::cout << "🧪 NST Lua Plugin CLI Simulation Tool\n";
    std::cout << "========================================================\n";

    QString scriptName = "nodenetwork_translate.lua";
    if (argc > 1) scriptName = QString::fromUtf8(argv[1]);

    QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts/lua/" + scriptName;
    if (!QFile::exists(scriptPath)) {
        scriptPath = "scripts/lua/" + scriptName;
    }

    std::cout << "[SIMULATOR] Target Script: " << scriptPath.toStdString() << "\n";

    // Read current settings from PluginSettings.ini
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
    QString apiKey = settings.value("Plugins/" + scriptName + "/Settings/api_key").toString();
    QString baseUrl = settings.value("Plugins/" + scriptName + "/Settings/base_url", "https://payg.nodenetwork.ovh").toString();
    QString format = settings.value("Plugins/" + scriptName + "/Settings/api_format", "OpenAI v1").toString();
    QString model = settings.value("Plugins/" + scriptName + "/Settings/model", "gemini-3.6-flash").toString();

    std::cout << "[SIMULATOR] Loaded Config:\n";
    std::cout << "  - API Key:    " << (apiKey.isEmpty() ? "(MISSING/EMPTY)" : (apiKey.left(8) + "...").toStdString()) << "\n";
    std::cout << "  - Base URL:   " << baseUrl.toStdString() << "\n";
    std::cout << "  - API Format: " << format.toStdString() << "\n";
    std::cout << "  - Model:      " << model.toStdString() << "\n";
    std::cout << "========================================================\n\n";

    LuaTranslationService service(scriptPath);

    QObject::connect(&service, &LuaTranslationService::translationFinished, [](const qtlingo::TranslationResult &res) {
        std::cout << "\n🟢 [SINGLE TRANSLATION RESULT]\n";
        std::cout << "   Source:     " << res.sourceText.toStdString() << "\n";
        std::cout << "   Translated: " << res.translatedText.toStdString() << "\n\n";
        QCoreApplication::quit();
    });

    QObject::connect(&service, &LuaTranslationService::batchTranslationFinished, [](const QList<qtlingo::TranslationResult> &results) {
        std::cout << "\n🟢 [BATCH TRANSLATION RESULT] (" << results.size() << " items):\n";
        for (const auto &res : results) {
            std::cout << "   - [" << res.sourceText.toStdString() << "] => [" << res.translatedText.toStdString() << "]\n";
        }
        std::cout << "\n";
        QCoreApplication::quit();
    });

    QObject::connect(&service, &LuaTranslationService::errorOccurred, [](const QString &err) {
        std::cout << "\n🔴 [TRANSLATION ERROR]\n   " << err.toStdString() << "\n\n";
        QCoreApplication::quit();
    });

    // Timeout safety after 15 seconds
    QTimer::singleShot(15000, []() {
        std::cout << "\n⚠️ [SIMULATION TIMEOUT] Exceeded 15 seconds limit.\n\n";
        QCoreApplication::quit();
    });

    std::cout << "🚀 Executing Single Translation: 'ヒロイン'...\n";
    service.translate("ヒロイン");

    return app.exec();
}
