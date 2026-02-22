#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QMap>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QFormLayout> // Ensure this is also included explicitly if not already by UI header
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    
    // Connect translator mode to stacked widget
    connect(ui->translatorModeComboBox, &QComboBox::currentIndexChanged, ui->engineStackedWidget, &QStackedWidget::setCurrentIndex);

    // Populate Source Language
    ui->sourceLanguageComboBox->addItem("Auto Detect", "auto");
    ui->sourceLanguageComboBox->addItem("English", "en");
    ui->sourceLanguageComboBox->addItem("Japanese", "ja");
    ui->sourceLanguageComboBox->addItem("Korean", "ko");
    ui->sourceLanguageComboBox->addItem("Chinese (Simplified)", "zh-CN");
    // Add more if needed

    // Populate with some languages
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
    
    // Connect fetch models button
    connect(ui->fetchModelsButton, &QPushButton::clicked, this, &SettingsDialog::fetchLlmModels);

    updateLlmModelComboBox();
    
    setupPluginsUI();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QString SettingsDialog::googleApiKey() const
{
    // Fix: Access line edit in new location (check object name in UI)
    // In new UI, it's still googleApiKeyEdit
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
    QString provider = ui->llmProviderComboBox->currentText();
    if (provider == "Google") return "Google";
    return provider;
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
    if (provider == "Google") providerText = "Google AI";
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
    updateLlmModelComboBox(); // Ensure the models are populated for the current provider
    if (ui->llmModelComboBox->isEditable()) {
        ui->llmModelComboBox->setCurrentText(model);
    } else {
        int index = ui->llmModelComboBox->findText(model);
        if (index != -1) {
            ui->llmModelComboBox->setCurrentIndex(index);
        } else if (!model.isEmpty()) {
            // Model was fetched dynamically before and is not in the default list, add it back.
            ui->llmModelComboBox->insertItem(0, model);
            ui->llmModelComboBox->setCurrentIndex(0);
        }
    }
}

bool SettingsDialog::isGoogleApi() const
{
    // Mode 0: Google Translate. We consider it "API" mode if there's a key provided.
    // Otherwise, we fall back to the free engine.
    return ui->translatorModeComboBox->currentIndex() == 0 && !ui->googleApiKeyEdit->text().trimmed().isEmpty();
}

void SettingsDialog::setGoogleApi(bool isApi)
{
    // If the user previously used any Google mode, set index to 0.
    // As we combined them, we just set it to Google Translate if they weren't using LLM or Plugins.
    if (ui->translatorModeComboBox->currentIndex() != 1 && ui->translatorModeComboBox->currentIndex() != 2) {
         ui->translatorModeComboBox->setCurrentIndex(0);
    }
}

// Helper to determine active mode more generically if needed
// 0: Quick, 1: Pro, 2: LLM, 3: Plugins
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

void SettingsDialog::updateConfigPanel()
{
    // Logic to enable/disable tabs or show specific pages is now handled by the Sidebar.
    // We might want to disable specific pages if they are not active? 
    // For now, let's allow the user to browse settings freely.
}

void SettingsDialog::updateLlmModelComboBox()
{
    QString provider = ui->llmProviderComboBox->currentText();
    ui->llmModelComboBox->clear();

    if (provider == "OpenAI") {
        ui->llmModelComboBox->addItems({"gpt-4o-mini", "gpt-4o", "o3-mini", "o1-mini", "gpt-4-turbo", "gpt-4", "gpt-3.5-turbo"});
    } else if (provider == "Anthropic") {
        ui->llmModelComboBox->addItems({"claude-3-opus-20240229", "claude-3-sonnet-20240229", "claude-3-haiku-20240307"});
    } else if (provider == "Google AI") {
        ui->llmModelComboBox->addItems({"gemini-1.5-pro-latest", "gemini-pro"});
    }
}

void SettingsDialog::fetchLlmModels()
{
    QString provider = ui->llmProviderComboBox->currentText();
    QString apiKey = ui->llmApiKeyEdit->text().trimmed();

    if (provider != "OpenAI") {
        QMessageBox::information(this, "Fetch Models", "Dynamic model fetching is currently only implemented for OpenAI.");
        return;
    }

    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, "Fetch Models", "Please enter an API Key first.");
        return;
    }

    // Disable button to prevent spamming
    ui->fetchModelsButton->setEnabled(false);
    ui->fetchModelsButton->setText("Fetching...");

    QNetworkRequest request(QUrl("https://api.openai.com/v1/models"));
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        ui->fetchModelsButton->setEnabled(true);
        ui->fetchModelsButton->setText("Fetch");

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject() && doc.object().contains("data")) {
                QJsonArray data = doc.object()["data"].toArray();
                QStringList models;
                for (const QJsonValue &val : data) {
                    if (val.isObject() && val.toObject().contains("id")) {
                        QString id = val.toObject()["id"].toString();
                        // Filter for typical chat/completion models to reduce clutter
                        if (id.startsWith("gpt-") || id.startsWith("o1") || id.startsWith("o3")) {
                            models.append(id);
                        }
                    }
                }
                
                if (!models.isEmpty()) {
                    models.sort();
                    ui->llmModelComboBox->clear();
                    
                    // Always put the most cost-effective recommendation at the top
                    if (models.contains("gpt-4o-mini")) {
                        models.removeAll("gpt-4o-mini");
                        ui->llmModelComboBox->addItem("gpt-4o-mini");
                    }
                    
                    ui->llmModelComboBox->addItems(models);
                    QMessageBox::information(this, "Fetch Models", QString("Successfully fetched %1 models.").arg(models.size() + 1));
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

// --- Plugin Manager Implementation ---

#include <QListWidget>
#include <QCheckBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QDir>
#include <QSettings>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <lua.hpp>
#include <QJsonDocument>
#include <QJsonObject>

void SettingsDialog::setupPluginsUI()
{
    // Removed RadioButton toggle connection as it's no longer used for page switching
    
    connect(ui->pluginListWidget, &QListWidget::itemClicked, this, &SettingsDialog::onPluginSelected);
    
    connect(ui->pluginEnabledCheckBox, &QCheckBox::toggled, [this](bool checked){
        if (ui->pluginListWidget->currentItem()) {
             QString scriptName = ui->pluginListWidget->currentItem()->text();
             QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
             settings.setValue("Plugins/" + scriptName + "/Enabled", checked);
        }
    });

    // Make sure we update the enabled state check
    ui->pluginEnabledCheckBox->setEnabled(false);
    
    loadPluginList();
}

void SettingsDialog::loadPluginList()
{
    QDir scriptDir(QCoreApplication::applicationDirPath());
    if (!scriptDir.cd("scripts")) {
        // Try one level up (for dev environment)
        scriptDir.cdUp();
        scriptDir.cd("scripts");
    }
    
    ui->pluginListWidget->clear();
    for (const QString &fileName : scriptDir.entryList({"*.lua"}, QDir::Files)) {
        // Filter logic (same as plugin)
        // For now, list all, or check for on_text_extract?
        // Let's check for on_text_extract to be consistent.
        QString filePath = scriptDir.absoluteFilePath(fileName);
        lua_State *L = luaL_newstate();
        if (luaL_dofile(L, filePath.toStdString().c_str()) == LUA_OK) {
            lua_getglobal(L, "on_text_extract");
            if (lua_isfunction(L, -1)) {
                 ui->pluginListWidget->addItem(fileName);
            }
        }
        lua_close(L);
    }
}

void SettingsDialog::onPluginSelected(QListWidgetItem *item)
{
    if (!item) return;
    QString scriptName = item->text();
    
    // Load Enabled State
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
    bool enabled = settings.value("Plugins/" + scriptName + "/Enabled", false).toBool();
    
    ui->pluginEnabledCheckBox->setEnabled(true);
    ui->pluginEnabledCheckBox->blockSignals(true);
    ui->pluginEnabledCheckBox->setChecked(enabled);
    ui->pluginEnabledCheckBox->blockSignals(false);
    
    // Load Settings Schema
    clearPluginSettingsUI();
    
    // Find script path
    QDir scriptDir(QCoreApplication::applicationDirPath());
    if (!scriptDir.cd("scripts")) { scriptDir.cdUp(); scriptDir.cd("scripts"); }
    QString filePath = scriptDir.absoluteFilePath(scriptName);
    
    QJsonArray schema = getPluginSettingsSchema(filePath);
    
    for (const QJsonValue &val : schema) {
        QJsonObject field = val.toObject();
        QString key = field["key"].toString();
        QString label = field["label"].toString();
        QString type = field["type"].toString();
        QString defaultValue = field["default"].toString(); // or toVariant
        
        QString currentVal = settings.value("Plugins/" + scriptName + "/Settings/" + key, defaultValue).toString();
        
        if (type == "text" || type == "password") {
            QLineEdit *edit = new QLineEdit();
            edit->setText(currentVal);
            if (type == "password") edit->setEchoMode(QLineEdit::Password);
            
            // Save on change
            connect(edit, &QLineEdit::textChanged, [scriptName, key](const QString &text){
                QSettings s(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
                s.setValue("Plugins/" + scriptName + "/Settings/" + key, text);
            });
            
            ui->formLayout_plugins->addRow(label + ":", edit);
        } else if (type == "dropdown") {
            QComboBox *combo = new QComboBox();
            QJsonArray options = field["options"].toArray();
            for (const QJsonValue &opt : options) {
                combo->addItem(opt.toString());
            }
            
            // Set current value
            int idx = combo->findText(currentVal);
            if (idx != -1) combo->setCurrentIndex(idx);
            else if (combo->count() > 0) combo->setCurrentIndex(0); // Default to first if not found
            
            // Save on change
            connect(combo, &QComboBox::currentTextChanged, [scriptName, key](const QString &text){
                QSettings s(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
                s.setValue("Plugins/" + scriptName + "/Settings/" + key, text);
            });
            
            ui->formLayout_plugins->addRow(label + ":", combo);
        }
    }
}

void SettingsDialog::clearPluginSettingsUI()
{
    QLayoutItem *item;
    while ((item = ui->formLayout_plugins->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

QJsonArray SettingsDialog::getPluginSettingsSchema(const QString &scriptPath)
{
    QJsonArray schema;
    lua_State *L = luaL_newstate();
    luaL_openlibs(L); // Needed for table manipulation if script uses it
    
    if (luaL_dofile(L, scriptPath.toStdString().c_str()) == LUA_OK) {
        lua_getglobal(L, "on_define_settings");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
                // Result should be a table (array of objects)
                // Convert Lua table to QJsonArray
                // Use a simplified converter here since we don't have the full helper from LuaWorker
                // Or just copy the helper?
                // Let's implement a quick one-off for this specific structure.
                
                if (lua_istable(L, -1)) {
                    int len = lua_rawlen(L, -1);
                    for (int i = 1; i <= len; ++i) {
                        lua_rawgeti(L, -1, i); // Push item
                        if (lua_istable(L, -1)) {
                            QJsonObject obj;
                            lua_pushnil(L);
                            while (lua_next(L, -2) != 0) {
                                QString k = QString::fromUtf8(lua_tostring(L, -2));
                                
                                // Check value type
                                if (lua_isstring(L, -1)) {
                                    QString v = QString::fromUtf8(lua_tostring(L, -1));
                                    obj.insert(k, v);
                                } else if (lua_istable(L, -1) && k == "options") {
                                    // Handle options array
                                    QJsonArray optionsArr;
                                    int optLen = lua_rawlen(L, -1);
                                    for (int j = 1; j <= optLen; ++j) {
                                        lua_rawgeti(L, -1, j);
                                        if (lua_isstring(L, -1)) {
                                            optionsArr.append(QString::fromUtf8(lua_tostring(L, -1)));
                                        }
                                        lua_pop(L, 1);
                                    }
                                    obj.insert("options", optionsArr);
                                }
                                
                                lua_pop(L, 1);
                            }
                            schema.append(obj);
                        }
                        lua_pop(L, 1); // Pop item
                    }
                }
            }
        }
    }
    lua_close(L);
    return schema;
}