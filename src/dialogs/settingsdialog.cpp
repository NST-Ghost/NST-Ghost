#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QMap>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QFormLayout>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QListWidget>
#include <QDir>
#include <QDirIterator>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QElapsedTimer>
#include <QDateTime>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <lua.hpp>

#include "src/llm_translation_service.h"

static QString resolveScriptPath(const QString &scriptName) {
    QDir dir(QCoreApplication::applicationDirPath());
    if (!dir.cd("scripts")) { dir.cdUp(); dir.cd("scripts"); }
    
    // 1. Prefer lua/ subfolder if exists
    QFileInfo luaSub(dir.absoluteFilePath("lua/" + scriptName));
    if (luaSub.exists()) return luaSub.absoluteFilePath();

    // 2. Search recursively across scripts directory (excluding templates)
    QDirIterator it(dir.absolutePath(), QStringList() << "*.lua", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (it.filePath().contains("/templates/") || it.filePath().contains("/template/")) {
            continue;
        }
        if (it.fileName() == scriptName || it.filePath().endsWith(scriptName)) {
            return it.filePath();
        }
    }
    return dir.absoluteFilePath(scriptName);
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    // Dynamic Top Section Header Label
    QLabel *headerTitleLabel = new QLabel("General Preferences", this);
    headerTitleLabel->setObjectName("settingsHeaderTitle");
    ui->verticalLayout_content->insertWidget(0, headerTitleLabel);

    connect(ui->settingsListWidget, &QListWidget::currentRowChanged, this, [this, headerTitleLabel](int row) {
        ui->configStackedWidget->setCurrentIndex(row);
        if (auto *item = ui->settingsListWidget->item(row)) {
            headerTitleLabel->setText(item->text());
        }
    });
    ui->settingsListWidget->setCurrentRow(0);

    // Sidebar Category Items Formatting
    if (ui->settingsListWidget->count() >= 4) {
        ui->settingsListWidget->item(0)->setText("General Settings");
        ui->settingsListWidget->item(1)->setText("Translation Engine");
        ui->settingsListWidget->item(2)->setText("AI Filter & Guard");
        ui->settingsListWidget->item(3)->setText("Plugins Manager");
    }

    // Populate LLM Providers
    ui->llmProviderComboBox->clear();
    ui->llmProviderComboBox->addItems({
        "NodeNetwork PAYG",
        "OpenAI",
        "MaxPlus AI",
        "DeepSeek",
        "Claude",
        "Google AI Studio",
        "Groq",
        "Custom (OpenAI-compatible)",
        "AI21",
        "AI/ML API",
        "Azure OpenAI",
        "Chutes",
        "Cohere",
        "Electron Hub",
        "Fireworks AI",
        "Google Vertex AI",
        "MistralAI",
        "Moonshot AI",
        "NanoGPT"
    });
    ui->llmModelComboBox->setEditable(true);

    // Connect translator mode to engine stacked widget
    connect(ui->translatorModeComboBox, &QComboBox::currentIndexChanged, ui->engineStackedWidget, &QStackedWidget::setCurrentIndex);

    // Populate Source Language
    ui->sourceLanguageComboBox->addItem("Auto Detect", "auto");
    ui->sourceLanguageComboBox->addItem("English", "en");
    ui->sourceLanguageComboBox->addItem("Japanese", "ja");
    ui->sourceLanguageComboBox->addItem("Korean", "ko");
    ui->sourceLanguageComboBox->addItem("Chinese (Simplified)", "zh-CN");

    // Populate Target Language
    ui->targetLanguageComboBox->addItem("English", "en");
    ui->targetLanguageComboBox->addItem("Spanish", "es");
    ui->targetLanguageComboBox->addItem("French", "fr");
    ui->targetLanguageComboBox->addItem("German", "de");
    ui->targetLanguageComboBox->addItem("Italian", "it");
    ui->targetLanguageComboBox->addItem("Portuguese", "pt");
    ui->targetLanguageComboBox->addItem("Russian", "ru");
    ui->targetLanguageComboBox->addItem("Chinese (Simplified)", "zh-CN");
    ui->targetLanguageComboBox->addItem("Japanese", "ja");
    ui->targetLanguageComboBox->addItem("Korean", "ko");
    ui->targetLanguageComboBox->addItem("Arabic", "ar");
    ui->targetLanguageComboBox->addItem("Hindi", "hi");
    ui->targetLanguageComboBox->addItem("Thai", "th");

    connect(ui->llmProviderComboBox, &QComboBox::currentIndexChanged, this, &SettingsDialog::updateLlmModelComboBox);
    connect(ui->fetchModelsButton, &QPushButton::clicked, this, &SettingsDialog::fetchLlmModels);
    connect(ui->fetchPluginModelsButton, &QPushButton::clicked, this, &SettingsDialog::fetchPluginModels);

    connect(ui->testConnectionBtn, &QPushButton::clicked, this, &SettingsDialog::testConnection);
    connect(ui->clearConsoleBtn, &QPushButton::clicked, [this]() {
        ui->devConsoleEdit->clear();
    });

    connect(ui->llmApiKeyEdit, &QLineEdit::textChanged, [this]() {
        updateLlmModelVisibility(true);
    });

    // Connect Lua Plugin ComboBox changes
    connect(ui->luaPluginComboBox, &QComboBox::currentTextChanged, this, &SettingsDialog::updateLuaPluginEngineUI);

    // Save changes for selected plugin
    connect(ui->pluginApiKeyEdit, &QLineEdit::textChanged, [this](const QString &text) {
        QString scriptName = ui->luaPluginComboBox->currentText();
        if (!scriptName.isEmpty()) {
            QSettings s(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
            s.setValue("Plugins/" + scriptName + "/Settings/api_key", text);
        }
        updatePluginModelVisibility(true);
    });

    connect(ui->pluginBaseUrlEdit, &QLineEdit::textChanged, [this](const QString &text) {
        QString scriptName = ui->luaPluginComboBox->currentText();
        if (!scriptName.isEmpty()) {
            QSettings s(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
            s.setValue("Plugins/" + scriptName + "/Settings/base_url", text);
        }
    });

    connect(ui->pluginModelComboBox, &QComboBox::currentTextChanged, [this](const QString &text) {
        QString scriptName = ui->luaPluginComboBox->currentText();
        if (!scriptName.isEmpty()) {
            QSettings s(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
            s.setValue("Plugins/" + scriptName + "/Settings/model", text);
        }
    });

    updateLlmModelComboBox();
    setupPluginsUI();
    updateLlmModelVisibility(false);
    updatePluginModelVisibility(false);

    // --- Parallel Translation Workers & Console Log Settings ---
    QGroupBox *parallelGroupBox = new QGroupBox("Performance & Parallel Translation Pipeline", this);
    QFormLayout *formLayout = new QFormLayout(parallelGroupBox);

    QComboBox *parallelWorkersCombo = new QComboBox(this);
    parallelWorkersCombo->addItem("1 Worker (Sequential / Safest)", 1);
    parallelWorkersCombo->addItem("2 Workers (Recommended / 2x Speed)", 2);
    parallelWorkersCombo->addItem("3 Workers (High Speed / 3x Speed)", 3);
    parallelWorkersCombo->addItem("4 Workers (Ultra Speed / 4x Speed)", 4);

    QSettings settings;
    int savedWorkers = settings.value("Translation/ParallelWorkers", 2).toInt();
    int comboIndex = parallelWorkersCombo->findData(savedWorkers);
    if (comboIndex >= 0) parallelWorkersCombo->setCurrentIndex(comboIndex);

    connect(parallelWorkersCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [parallelWorkersCombo]() {
        int workers = parallelWorkersCombo->currentData().toInt();
        QSettings s;
        s.setValue("Translation/ParallelWorkers", workers);
    });

    QCheckBox *showConsoleCheckBox = new QCheckBox("Show Live Detailed Progress Console Modal", this);
    showConsoleCheckBox->setChecked(settings.value("General/ShowDetailedProgressConsole", true).toBool());
    connect(showConsoleCheckBox, &QCheckBox::toggled, [](bool checked) {
        QSettings s;
        s.setValue("General/ShowDetailedProgressConsole", checked);
    });

    formLayout->addRow("Parallel Worker Channels:", parallelWorkersCombo);
    formLayout->addRow("Console Log Overlay:", showConsoleCheckBox);

    if (ui->page_general && ui->page_general->layout()) {
        ui->page_general->layout()->addWidget(parallelGroupBox);
    }
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QString SettingsDialog::googleApiKey() const
{
    return ui->googleApiKeyEdit->text();
}

QString SettingsDialog::sourceLanguage() const
{
    return ui->sourceLanguageComboBox->currentData().toString();
}

QString SettingsDialog::targetLanguage() const
{
    return ui->targetLanguageComboBox->currentData().toString();
}

QString SettingsDialog::targetLanguageName() const
{
    return ui->targetLanguageComboBox->currentText();
}

QString SettingsDialog::llmProvider() const
{
    return ui->llmProviderComboBox->currentText();
}

QString SettingsDialog::llmApiKey() const
{
    return ui->llmApiKeyEdit->text();
}

QString SettingsDialog::llmBaseUrl() const
{
    return ui->llmBaseUrlEdit->text();
}

QString SettingsDialog::llmModel() const
{
    return ui->llmModelComboBox->currentText();
}

bool SettingsDialog::isRelationsEnabled() const
{
    return ui->enableRelationsCheckBox->isChecked();
}

void SettingsDialog::setRelationsEnabled(bool enabled)
{
    ui->enableRelationsCheckBox->setChecked(enabled);
}

bool SettingsDialog::isAiFilterEnabled() const
{
    return ui->enableAiFilterCheckBox->isChecked();
}

double SettingsDialog::aiFilterThreshold() const
{
    return ui->aiFilterThresholdSpinBox->value();
}

void SettingsDialog::setAiFilterEnabled(bool enabled)
{
    ui->enableAiFilterCheckBox->setChecked(enabled);
}

void SettingsDialog::setAiFilterThreshold(double threshold)
{
    ui->aiFilterThresholdSpinBox->setValue(threshold);
}

bool SettingsDialog::isBackupEnabled() const
{
    return ui->enableBackupCheckBox->isChecked();
}

void SettingsDialog::setBackupEnabled(bool enabled)
{
    ui->enableBackupCheckBox->setChecked(enabled);
}

void SettingsDialog::setGoogleApiKey(const QString &apiKey)
{
    ui->googleApiKeyEdit->setText(apiKey);
}

void SettingsDialog::setSourceLanguage(const QString &language)
{
    int index = ui->sourceLanguageComboBox->findData(language);
    if (index != -1) {
        ui->sourceLanguageComboBox->setCurrentIndex(index);
    }
}

void SettingsDialog::setTargetLanguage(const QString &language)
{
    int index = ui->targetLanguageComboBox->findData(language);
    if (index != -1) {
        ui->targetLanguageComboBox->setCurrentIndex(index);
    }
}

void SettingsDialog::setLlmProvider(const QString &provider)
{
    QString providerText = provider;
    if (provider == "Anthropic") providerText = "Claude";
    if (provider == "Google" || provider == "Google AI") providerText = "Google AI Studio";
    int index = ui->llmProviderComboBox->findText(providerText);
    if (index != -1) {
        ui->llmProviderComboBox->setCurrentIndex(index);
    }
}

void SettingsDialog::setLlmApiKey(const QString &apiKey)
{
    ui->llmApiKeyEdit->setText(apiKey);
}

void SettingsDialog::setLlmBaseUrl(const QString &baseUrl)
{
    ui->llmBaseUrlEdit->setText(baseUrl);
}

void SettingsDialog::setLlmModel(const QString &model)
{
    updateLlmModelComboBox();
    if (ui->llmModelComboBox->isEditable()) {
        ui->llmModelComboBox->setCurrentText(model);
    } else {
        int index = ui->llmModelComboBox->findText(model);
        if (index != -1) {
            ui->llmModelComboBox->setCurrentIndex(index);
        } else if (!model.isEmpty()) {
            ui->llmModelComboBox->insertItem(0, model);
            ui->llmModelComboBox->setCurrentIndex(0);
        }
    }
}

bool SettingsDialog::isGoogleApi() const
{
    return ui->translatorModeComboBox->currentIndex() == 0 && !ui->googleApiKeyEdit->text().trimmed().isEmpty();
}

void SettingsDialog::setGoogleApi(bool isApi)
{
    if (ui->translatorModeComboBox->currentIndex() != 1 && ui->translatorModeComboBox->currentIndex() != 2) {
         ui->translatorModeComboBox->setCurrentIndex(0);
    }
}

int SettingsDialog::translationMode() const
{
    return ui->translatorModeComboBox->currentIndex();
}

void SettingsDialog::setTranslationMode(int mode)
{
    if (mode >= 0 && mode < ui->translatorModeComboBox->count()) {
        ui->translatorModeComboBox->setCurrentIndex(mode);
    }
}

void SettingsDialog::updateLlmModelComboBox()
{
    QString provider = ui->llmProviderComboBox->currentText();
    ui->llmModelComboBox->clear();
    if (ui->llmModelComboBox->view()) {
        ui->llmModelComboBox->view()->setMinimumWidth(380);
    }

    if (provider == "NodeNetwork PAYG") {
        ui->llmModelComboBox->addItems({
            "gemini-3.6-flash",
            "gemini-3.5-flash",
            "gemini-3.1-flash",
            "gemini-3.1-pro",
            "gpt-5.6-luna",
            "gpt-5.6-sol",
            "gpt-5.6-terra",
            "gpt-5.5",
            "gpt-5.5-instant",
            "gpt-5.4",
            "gpt-5.4-mini",
            "gpt-5.4-nano",
            "gpt-5.3-codex",
            "gpt-5.3-codex-spark",
            "claude-sonnet-5",
            "claude-opus-5",
            "claude-opus-4.8",
            "claude-haiku-4.5",
            "claude-fable-5",
            "deepseek-v4-flash",
            "deepseek-v4-pro",
            "glm-5.2",
            "grok-4.5",
            "mimo-v2.5",
            "mimo-v2.5-pro",
            "minimax-m3",
            "qwen-3.7-max",
            "qwen-3.7-plus"
        });
        ui->llmBaseUrlEdit->setPlaceholderText("https://payg.nodenetwork.ovh");
    } else if (provider == "OpenAI") {
        ui->llmModelComboBox->addItems({"gpt-4o-mini", "gpt-4o", "gpt-4.1-mini", "gpt-4.1", "o3-mini", "o1-mini"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default: https://api.openai.com/v1)");
    } else if (provider == "Claude") {
        ui->llmModelComboBox->addItems({"claude-3-5-sonnet-latest", "claude-3-5-haiku-latest", "claude-3-opus-latest"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default: https://api.anthropic.com/v1/messages)");
    } else if (provider == "Google AI Studio") {
        ui->llmModelComboBox->addItems({"gemini-2.0-flash", "gemini-1.5-flash", "gemini-1.5-pro"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default Google AI Studio endpoint)");
    } else if (provider == "Google Vertex AI") {
        ui->llmModelComboBox->addItems({"gemini-1.5-flash", "gemini-1.5-pro"});
        ui->llmBaseUrlEdit->setPlaceholderText("Full Vertex AI generateContent endpoint URL");
    } else if (provider == "MaxPlus AI") {
        ui->llmModelComboBox->addItems({"gpt-4o", "gpt-4o-mini", "claude-3-5-sonnet", "deepseek-v3", "deepseek-r1"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default: https://api.maxplus-ai.cc/v1)");
    } else if (provider == "Groq") {
        ui->llmModelComboBox->addItems({"llama-3.3-70b-versatile", "llama-3.1-8b-instant", "mixtral-8x7b-32768"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default: https://api.groq.com/openai/v1)");
    } else if (provider == "DeepSeek") {
        ui->llmModelComboBox->addItems({"deepseek-chat", "deepseek-reasoner"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default: https://api.deepseek.com)");
    } else if (provider == "MistralAI") {
        ui->llmModelComboBox->addItems({"mistral-large-latest", "mistral-small-latest", "open-mistral-nemo"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default: https://api.mistral.ai/v1)");
    } else if (provider == "Cohere") {
        ui->llmModelComboBox->addItems({"command-a-03-2025", "command-r-plus", "command-r"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default: https://api.cohere.ai/compatibility/v1)");
    } else if (provider == "Fireworks AI") {
        ui->llmModelComboBox->addItems({"accounts/fireworks/models/llama-v3p1-8b-instruct", "accounts/fireworks/models/llama-v3p1-70b-instruct"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default: https://api.fireworks.ai/inference/v1)");
    } else if (provider == "Moonshot AI") {
        ui->llmModelComboBox->addItems({"moonshot-v1-8k", "moonshot-v1-32k", "moonshot-v1-128k"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default: https://api.moonshot.ai/v1)");
    } else if (provider == "AI21") {
        ui->llmModelComboBox->addItems({"jamba-large", "jamba-mini"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default: https://api.ai21.com/studio/v1)");
    } else if (provider == "AI/ML API") {
        ui->llmModelComboBox->addItems({"deepseek/deepseek-chat", "meta-llama/Llama-3.3-70B-Instruct-Turbo", "mistralai/Mistral-Large-Instruct-2411"});
        ui->llmBaseUrlEdit->setPlaceholderText("(Leave empty for default: https://api.aimlapi.com/v1)");
    } else {
        ui->llmBaseUrlEdit->setPlaceholderText("Enter API Base URL (e.g. http://localhost:11434/v1)");
    }

    if (ui->llmModelComboBox->count() == 0) {
        ui->llmModelComboBox->setEditText(QString());
    }
}

void SettingsDialog::fetchLlmModels()
{
    QString provider = ui->llmProviderComboBox->currentText();
    QString apiKey = ui->llmApiKeyEdit->text().trimmed();

    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, "Fetch Models", "Please enter an API Key first.");
        return;
    }

    QString baseUrl = llmBaseUrl().trimmed();
    if (baseUrl.isEmpty()) {
        if (provider == "OpenAI") baseUrl = "https://api.openai.com/v1";
        else if (provider == "MaxPlus AI") baseUrl = "https://api.maxplus-ai.cc/v1";
        else if (provider == "Groq") baseUrl = "https://api.groq.com/openai/v1";
        else if (provider == "DeepSeek") baseUrl = "https://api.deepseek.com";
        else if (provider == "MistralAI") baseUrl = "https://api.mistral.ai/v1";
        else if (provider == "AI/ML API") baseUrl = "https://api.aimlapi.com/v1";
        else if (provider == "NodeNetwork PAYG") baseUrl = "https://payg.nodenetwork.ovh";
        else {
            QMessageBox::warning(this, "Fetch Models", "Automatic model fetching is currently supported for OpenAI-compatible providers.");
            return;
        }
    }

    baseUrl = baseUrl.trimmed();
    if (baseUrl.endsWith("/")) baseUrl.chop(1);
    QUrl url(baseUrl.endsWith("/v1") ? baseUrl + "/models" : baseUrl + "/v1/models");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject() && doc.object().contains("data")) {
                QJsonArray data = doc.object()["data"].toArray();
                QStringList models;
                for (const QJsonValue &val : data) {
                    if (val.isObject() && val.toObject().contains("id")) {
                        QString id = val.toObject()["id"].toString();
                        if (!id.isEmpty()) {
                            models.append(id);
                        }
                    }
                }
                
                if (!models.isEmpty()) {
                    models.sort();
                    ui->llmModelComboBox->clear();
                    ui->llmModelComboBox->addItems(models);
                    QMessageBox::information(this, "Fetch Models", QString("Successfully fetched %1 models.").arg(models.size()));
                } else {
                    QMessageBox::warning(this, "Fetch Models", "No compatible models found in the API response.");
                }
            } else {
                 QMessageBox::warning(this, "Fetch Models", "Failed to parse API response.");
            }
        } else {
            QMessageBox::warning(this, "Fetch Models", "Network Error: " + reply->errorString());
        }
        reply->deleteLater();
    });
}

void SettingsDialog::fetchPluginModels()
{
    QString scriptName = ui->luaPluginComboBox->currentText();
    QString apiKey = ui->pluginApiKeyEdit->text().trimmed();
    QString baseUrl = ui->pluginBaseUrlEdit->text().trimmed();

    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, "Fetch Models", "Please enter an API Key first.");
        return;
    }

    if (baseUrl.isEmpty()) {
        baseUrl = ui->pluginBaseUrlEdit->placeholderText();
        if (baseUrl.contains("http")) {
            int start = baseUrl.indexOf("http");
            int end = baseUrl.indexOf(")", start);
            if (end == -1) end = baseUrl.length();
            baseUrl = baseUrl.mid(start, end - start).trimmed();
        }
    }

    if (!baseUrl.isEmpty() && baseUrl.startsWith("http")) {
        baseUrl = baseUrl.trimmed();
        if (baseUrl.endsWith("/")) baseUrl.chop(1);
        QUrl url(baseUrl.endsWith("/v1") ? baseUrl + "/models" : baseUrl + "/v1/models");
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());

        QNetworkReply *reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, scriptName]() {
            if (reply->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                if (doc.isObject() && doc.object().contains("data")) {
                    QJsonArray data = doc.object()["data"].toArray();
                    QStringList models;
                    for (const QJsonValue &val : data) {
                        if (val.isObject() && val.toObject().contains("id")) {
                            QString id = val.toObject()["id"].toString();
                            if (!id.isEmpty()) {
                                models.append(id);
                            }
                        }
                    }
                    
                    if (!models.isEmpty()) {
                        models.sort();
                        ui->pluginModelComboBox->clear();
                        ui->pluginModelComboBox->addItems(models);
                        QMessageBox::information(this, "Fetch Models", QString("Successfully fetched %1 models for %2.").arg(models.size()).arg(scriptName));
                        reply->deleteLater();
                        return;
                    }
                }
            }
            // Fallback to updateLuaPluginEngineUI
            updateLuaPluginEngineUI(scriptName);
            reply->deleteLater();
        });
    } else {
        updateLuaPluginEngineUI(scriptName);
    }
}

void SettingsDialog::updateLlmModelVisibility(bool animate)
{
    bool hasKey = !ui->llmApiKeyEdit->text().trimmed().isEmpty();
    QWidget* label = ui->llmModelLabel;
    QWidget* box = ui->llmModelComboBox;
    QWidget* btn = ui->fetchModelsButton;

    if (!hasKey) {
        label->setVisible(false);
        box->setVisible(false);
        btn->setVisible(false);
    } else {
        if (!box->isVisible()) {
            label->setVisible(true);
            box->setVisible(true);
            btn->setVisible(true);
            if (animate) {
                for (QWidget* w : {label, box, btn}) {
                    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(w);
                    w->setGraphicsEffect(effect);
                    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
                    anim->setDuration(300);
                    anim->setStartValue(0.0);
                    anim->setEndValue(1.0);
                    connect(anim, &QPropertyAnimation::finished, effect, &QObject::deleteLater);
                    anim->start(QAbstractAnimation::DeleteWhenStopped);
                }
            }
        }
    }
}

void SettingsDialog::updatePluginModelVisibility(bool animate)
{
    bool hasKey = !ui->pluginApiKeyEdit->text().trimmed().isEmpty();
    QWidget* label = ui->label_pluginModel;
    QWidget* box = ui->pluginModelComboBox;
    QWidget* btn = ui->fetchPluginModelsButton;

    if (!hasKey) {
        label->setVisible(false);
        box->setVisible(false);
        btn->setVisible(false);
    } else {
        if (!box->isVisible()) {
            label->setVisible(true);
            box->setVisible(true);
            btn->setVisible(true);
            if (animate) {
                for (QWidget* w : {label, box, btn}) {
                    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(w);
                    w->setGraphicsEffect(effect);
                    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
                    anim->setDuration(300);
                    anim->setStartValue(0.0);
                    anim->setEndValue(1.0);
                    connect(anim, &QPropertyAnimation::finished, effect, &QObject::deleteLater);
                    anim->start(QAbstractAnimation::DeleteWhenStopped);
                }
            }
        }
    }
}

void SettingsDialog::updateLuaPluginEngineUI(const QString &scriptName)
{
    if (scriptName.isEmpty()) return;

    QSignalBlocker modelBlocker(ui->pluginModelComboBox);
    QSignalBlocker apiKeyBlocker(ui->pluginApiKeyEdit);
    QSignalBlocker baseUrlBlocker(ui->pluginBaseUrlEdit);

    ui->pluginModelComboBox->clear();
    if (ui->pluginModelComboBox->view()) {
        ui->pluginModelComboBox->view()->setMinimumWidth(380);
    }
    QString filePath = resolveScriptPath(scriptName);

    // Load saved settings for this script first
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
    settings.setValue("Plugins/" + scriptName + "/Enabled", true);
    settings.setValue("Plugins/" + scriptName + "/Installed", true);
    settings.setValue("ActiveLuaPlugin", scriptName);

    QString savedApiKey = settings.value("Plugins/" + scriptName + "/Settings/api_key", "").toString();

    QString defaultBaseUrl = "";
    QStringList modelList;

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    // Register dummy API functions
    lua_register(L, "nst_http_request", [](lua_State* L) -> int {
        lua_pushstring(L, "");
        lua_pushinteger(L, 0);
        return 2;
    });

    if (luaL_dofile(L, filePath.toStdString().c_str()) == LUA_OK) {
        // 1. Check on_get_models() hook first
        lua_getglobal(L, "on_get_models");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 1, 0) == LUA_OK && lua_istable(L, -1)) {
                int len = lua_rawlen(L, -1);
                for (int i = 1; i <= len; ++i) {
                    lua_rawgeti(L, -1, i);
                    if (lua_isstring(L, -1)) {
                        modelList.append(QString::fromUtf8(lua_tostring(L, -1)));
                    }
                    lua_pop(L, 1);
                }
            }
        }

        // 2. Check on_define_settings() hook
        lua_getglobal(L, "on_define_settings");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 1, 0) == LUA_OK && lua_istable(L, -1)) {
                int len = lua_rawlen(L, -1);
                for (int i = 1; i <= len; ++i) {
                    lua_rawgeti(L, -1, i);
                    if (lua_istable(L, -1)) {
                        QString key, defVal;
                        QStringList options;

                        lua_pushnil(L);
                        while (lua_next(L, -2) != 0) {
                            QString k = QString::fromUtf8(lua_tostring(L, -2));
                            if (lua_isstring(L, -1)) {
                                QString v = QString::fromUtf8(lua_tostring(L, -1));
                                if (k == "key") key = v;
                                else if (k == "default") defVal = v;
                            } else if (lua_istable(L, -1) && k == "options") {
                                int optLen = lua_rawlen(L, -1);
                                for (int j = 1; j <= optLen; ++j) {
                                    lua_rawgeti(L, -1, j);
                                    if (lua_isstring(L, -1)) {
                                        options.append(QString::fromUtf8(lua_tostring(L, -1)));
                                    }
                                    lua_pop(L, 1);
                                }
                            }
                            lua_pop(L, 1);
                        }

                        if (key == "base_url" && !defVal.isEmpty()) {
                            defaultBaseUrl = defVal;
                        }
                        if (key == "model" && modelList.isEmpty() && !options.isEmpty()) {
                            modelList = options;
                        }
                    }
                    lua_pop(L, 1);
                }
            }
        }
    }
    lua_close(L);

    if (!modelList.isEmpty()) {
        ui->pluginModelComboBox->addItems(modelList);
    }

    QString savedBaseUrl = settings.value("Plugins/" + scriptName + "/Settings/base_url", defaultBaseUrl).toString();
    QString savedModel = settings.value("Plugins/" + scriptName + "/Settings/model", "").toString();

    ui->pluginApiKeyEdit->setText(savedApiKey);
    ui->pluginBaseUrlEdit->setText(savedBaseUrl);
    ui->pluginBaseUrlEdit->setPlaceholderText(defaultBaseUrl.isEmpty() ? "Default Endpoint URL" : defaultBaseUrl);

    if (!savedModel.isEmpty()) {
        int idx = ui->pluginModelComboBox->findText(savedModel);
        if (idx != -1) {
            ui->pluginModelComboBox->setCurrentIndex(idx);
        } else {
            ui->pluginModelComboBox->setEditText(savedModel);
        }
    } else if (ui->pluginModelComboBox->count() > 0) {
        ui->pluginModelComboBox->setCurrentIndex(0);
    }
}

// --- Plugins Manager Implementation ---

void SettingsDialog::setupPluginsUI()
{
    connect(ui->pluginListWidget, &QListWidget::itemClicked, this, &SettingsDialog::onPluginSelected);
    
    connect(ui->pluginEnabledCheckBox, &QCheckBox::toggled, [this](bool checked){
        if (ui->pluginListWidget->currentItem()) {
             QString scriptName = ui->pluginListWidget->currentItem()->text();
             QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
             settings.setValue("Plugins/" + scriptName + "/Enabled", checked);
        }
    });

    connect(ui->reloadPluginsButton, &QPushButton::clicked, this, &SettingsDialog::loadPluginList);

    ui->pluginEnabledCheckBox->setEnabled(false);
    loadPluginList();
}

void SettingsDialog::loadPluginList()
{
    QDir scriptDir(QCoreApplication::applicationDirPath());
    if (!scriptDir.cd("scripts")) {
        scriptDir.cdUp();
        scriptDir.cd("scripts");
    }
    
    ui->pluginListWidget->clear();
    ui->luaPluginComboBox->clear();

    QDirIterator it(scriptDir.absolutePath(), QStringList() << "*.lua", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        
        // Skip template folders and example scripts
        if (filePath.contains("/templates/") || filePath.contains("/template/")) {
            continue;
        }

        QString fileName = it.fileName();

        lua_State *L = luaL_newstate();
        luaL_openlibs(L);
        if (luaL_dofile(L, filePath.toStdString().c_str()) == LUA_OK) {
            lua_getglobal(L, "on_text_extract");
            if (lua_isfunction(L, -1)) {
                if (ui->luaPluginComboBox->findText(fileName) == -1) {
                    ui->pluginListWidget->addItem(fileName);
                    ui->luaPluginComboBox->addItem(fileName);
                }
            }
        }
        lua_close(L);
    }

    if (ui->luaPluginComboBox->count() > 0) {
        updateLuaPluginEngineUI(ui->luaPluginComboBox->currentText());
    }

    if (ui->pluginListWidget->count() > 0) {
        ui->pluginListWidget->setCurrentRow(0);
        onPluginSelected(ui->pluginListWidget->item(0));
    }
}

QString SettingsDialog::parsePluginMetadata(const QString &scriptPath)
{
    QFile file(scriptPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "Unable to open script file.";
    }

    QTextStream in(&file);
    QString name = "Unknown", version = "1.0", author = "Unknown", desc = "No description provided.";
    
    int lineCount = 0;
    while (!in.atEnd() && lineCount < 20) {
        QString line = in.readLine().trimmed();
        lineCount++;
        if (line.startsWith("-- Name:")) name = line.mid(8).trimmed();
        else if (line.startsWith("-- Version:")) version = line.mid(11).trimmed();
        else if (line.startsWith("-- Author:")) author = line.mid(10).trimmed();
        else if (line.startsWith("-- Description:")) desc = line.mid(15).trimmed();
    }

    return QString("<b>Name:</b> %1<br>"
                   "<b>Version:</b> %2<br>"
                   "<b>Author:</b> %3<br><br>"
                   "<b>Description:</b><br>%4")
            .arg(name, version, author, desc);
}

void SettingsDialog::onPluginSelected(QListWidgetItem *item)
{
    if (!item) return;
    QString scriptName = item->text();
    
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
    bool enabled = settings.value("Plugins/" + scriptName + "/Enabled", false).toBool();
    
    ui->pluginEnabledCheckBox->setEnabled(true);
    ui->pluginEnabledCheckBox->blockSignals(true);
    ui->pluginEnabledCheckBox->setChecked(enabled);
    ui->pluginEnabledCheckBox->blockSignals(false);

    QString filePath = resolveScriptPath(scriptName);
    ui->pluginInfoLabel->setText(parsePluginMetadata(filePath));
}

void SettingsDialog::testConnection()
{
    int mode = translationMode();
    ui->devConsoleEdit->clear();
    QElapsedTimer timer;
    timer.start();

    QString apiKey, baseUrl, model, provider;
    
    if (mode == 2) {
        // Lua Plugin Engine
        provider = ui->luaPluginComboBox->currentText();
        apiKey = ui->pluginApiKeyEdit->text().trimmed();
        baseUrl = ui->pluginBaseUrlEdit->text().trimmed();
        model = ui->pluginModelComboBox->currentText().trimmed();
        
        if (!provider.isEmpty()) {
            QSettings s(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
            s.setValue("Plugins/" + provider + "/Settings/api_key", apiKey);
            s.setValue("Plugins/" + provider + "/Settings/base_url", baseUrl);
            s.setValue("Plugins/" + provider + "/Settings/model", model);
        }
        
        if (baseUrl.isEmpty()) baseUrl = "https://payg.nodenetwork.ovh";
        if (model.isEmpty()) model = "claude-opus-5";
    } else {
        // LLM Engine
        provider = llmProvider();
        apiKey = llmApiKey().trimmed();
        baseUrl = llmBaseUrl().trimmed();
        model = llmModel().trimmed();

        if (provider == "NodeNetwork PAYG" && baseUrl.isEmpty()) {
            baseUrl = "https://payg.nodenetwork.ovh";
        }
    }

    if (apiKey.isEmpty() && provider != "Google Translate") {
        ui->devConsoleEdit->append("<font color='#ff5555'><b>[ERROR] API Key is missing. Please enter an API Key first.</b></font>");
        QMessageBox::warning(this, "Test Connection", "Please enter an API Key first.");
        return;
    }

    baseUrl = baseUrl.trimmed();
    if (baseUrl.endsWith("/")) baseUrl.chop(1);
    QUrl targetUrl(baseUrl.endsWith("/v1") ? baseUrl + "/chat/completions" : baseUrl + "/v1/chat/completions");

    // Mask API Key for security output
    QString maskedKey = apiKey;
    if (maskedKey.length() > 12) {
        maskedKey = maskedKey.left(8) + "..." + maskedKey.right(4);
    }

    // Construct Payload JSON
    QJsonObject sysMsg, userMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = "You are a professional game translator. Translate text to Thai. Keep control codes, tags, and formatting intact. Return ONLY translated text.";
    userMsg["role"] = "user";
    userMsg["content"] = "Say hello in one short sentence.";

    QJsonArray messages;
    messages.append(sysMsg);
    messages.append(userMsg);

    QJsonObject payloadObj;
    payloadObj["model"] = model;
    payloadObj["messages"] = messages;
    payloadObj["temperature"] = 0.3;

    QByteArray payloadData = QJsonDocument(payloadObj).toJson(QJsonDocument::Indented);

    // Render Outbound Request Log
    QString reqLog = QString(
        "<font color='#55ff55'><b>================================================================================</b></font><br>"
        "<font color='#55ff55'><b>[OUTBOUND REQUEST SENT]</b></font><br>"
        "<b>Timestamp:</b> %1<br>"
        "<b>Provider / Script:</b> %2<br>"
        "<b>Target Endpoint URL:</b> %3<br>"
        "<b>HTTP Method:</b> POST<br><br>"
        "<b>--- HTTP HEADERS ---</b><br>"
        "Content-Type: application/json<br>"
        "Authorization: Bearer %4<br><br>"
        "<b>--- REQUEST PAYLOAD JSON ---</b><br>"
        "<pre>%5</pre>"
        "<font color='#55ff55'><b>================================================================================</b></font><br>"
    ).arg(
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"),
        provider,
        targetUrl.toString(),
        maskedKey,
        QString::fromUtf8(payloadData).toHtmlEscaped()
    );

    ui->devConsoleEdit->append(reqLog);

    QNetworkRequest request(targetUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());

    QNetworkReply *reply = m_networkManager->post(request, payloadData);
    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, provider]() {
        qint64 elapsedMs = timer.elapsed();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray respBody = reply->readAll();

        QString rawHeadersStr;
        for (const QByteArray &h : reply->rawHeaderList()) {
            rawHeadersStr += QString("%1: %2\n").arg(QString(h), QString(reply->rawHeader(h)));
        }

        QJsonDocument respDoc = QJsonDocument::fromJson(respBody);
        QString prettyResp = respDoc.isEmpty() ? QString::fromUtf8(respBody) : QString::fromUtf8(respDoc.toJson(QJsonDocument::Indented));

        if (reply->error() == QNetworkReply::NoError) {
            QString respLog = QString(
                "<font color='#55ffff'><b>================================================================================</b></font><br>"
                "<font color='#55ffff'><b>[SERVER RESPONSE RECEIVED] (Elapsed: %1 ms)</b></font><br>"
                "<b>HTTP Status Code:</b> %2 OK<br><br>"
                "<b>--- RAW RESPONSE HEADERS ---</b><br>"
                "<pre>%3</pre>"
                "<b>--- SERVER RESPONSE BODY JSON ---</b><br>"
                "<pre>%4</pre>"
                "<font color='#55ffff'><b>================================================================================</b></font><br>"
                "<font color='#55ff55'><b>[TEST PASSED] Successfully connected to %5!</b></font><br>"
            ).arg(
                QString::number(elapsedMs),
                QString::number(statusCode),
                rawHeadersStr.toHtmlEscaped(),
                prettyResp.toHtmlEscaped(),
                provider
            );
            ui->devConsoleEdit->append(respLog);
            QMessageBox::information(this, "Test Connection", QString("[OK] Connection Successful!\n\nProvider: %1\nElapsed: %2 ms").arg(provider).arg(elapsedMs));
        } else {
            QString errLog = QString(
                "<font color='#ff5555'><b>================================================================================</b></font><br>"
                "<font color='#ff5555'><b>[SERVER RESPONSE ERROR] (Elapsed: %1 ms)</b></font><br>"
                "<b>HTTP Status Code:</b> %2 (%3)<br><br>"
                "<b>--- RAW RESPONSE HEADERS ---</b><br>"
                "<pre>%4</pre>"
                "<b>--- SERVER RESPONSE BODY ---</b><br>"
                "<pre>%5</pre>"
                "<font color='#ff5555'><b>================================================================================</b></font><br>"
                "<font color='#ff5555'><b>[TEST FAILED] %6</b></font><br>"
            ).arg(
                QString::number(elapsedMs),
                QString::number(statusCode),
                reply->errorString(),
                rawHeadersStr.toHtmlEscaped(),
                prettyResp.toHtmlEscaped(),
                reply->errorString()
            );
            ui->devConsoleEdit->append(errLog);
            QMessageBox::critical(this, "Test Connection", QString("[FAIL] Connection Failed!\n\nProvider: %1\nError: %2").arg(provider, reply->errorString()));
        }
        reply->deleteLater();
    });
}
