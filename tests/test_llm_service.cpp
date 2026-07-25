#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QMap>
#include "qtlingo/translationsettings.h"
#include "src/llm_translation_service.h"

class LLMServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        int argc = 1;
        static char appName[] = "LLMTest";
        static char* argv[] = { appName, nullptr };
        if (!QCoreApplication::instance()) {
            app = new QCoreApplication(argc, argv);
        }
    }
    QCoreApplication* app = nullptr;
};

TEST_F(LLMServiceTest, SettingsAndGlossary) {
    TranslationSettings settings;
    settings.llmProvider = "OpenAI";
    settings.llmApiKey = "test-key";
    settings.llmModel = "gpt-4o-mini";
    settings.glossary["ルフィ"] = "Luffy";
    settings.glossary["ポーション"] = "Potion";

    qtlingo::LLMTranslationService service;
    service.configure(settings);

    EXPECT_EQ(service.glossary().value("ルフィ"), "Luffy");
    EXPECT_EQ(service.glossary().value("ポーション"), "Potion");
    EXPECT_TRUE(service.isResponseCacheEnabled());
}

TEST_F(LLMServiceTest, ResponseCacheManagement) {
    qtlingo::LLMTranslationService service;
    service.setTargetLanguage("th");
    service.setSourceLanguage("ja");
    service.setApiKey("mock-key");
    service.setLlmProvider("OpenAI");
    service.setLlmModel("gpt-4o-mini");

    EXPECT_TRUE(service.isResponseCacheEnabled());
    service.setResponseCacheEnabled(false);
    EXPECT_FALSE(service.isResponseCacheEnabled());
    service.setResponseCacheEnabled(true);

    service.clearResponseCache();
}
