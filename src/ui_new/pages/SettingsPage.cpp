#include "SettingsPage.h"
#include "../styles/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QScrollArea>

namespace nst::ui {

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void SettingsPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Theme::Spacing::lg(), Theme::Spacing::lg(),
                                    Theme::Spacing::lg(), Theme::Spacing::lg());
    mainLayout->setSpacing(Theme::Spacing::lg());
    
    // Header
    QLabel *title = new QLabel("Settings", this);
    title->setStyleSheet(QString("font-size: 24px; font-weight: 600; color: %1;")
                         .arg(Theme::Colors::textPrimary().name()));
    mainLayout->addWidget(title);
    
    // Scroll area for settings
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget *scrollContent = new QWidget(scrollArea);
    QVBoxLayout *settingsLayout = new QVBoxLayout(scrollContent);
    settingsLayout->setSpacing(Theme::Spacing::lg());
    
    // Translation Settings Group
    QGroupBox *translationGroup = new QGroupBox("Translation", scrollContent);
    QFormLayout *translationForm = new QFormLayout(translationGroup);
    translationForm->setSpacing(Theme::Spacing::md());
    
    m_editApiKey = new QLineEdit(translationGroup);
    m_editApiKey->setEchoMode(QLineEdit::Password);
    m_editApiKey->setPlaceholderText("Enter your Google Translate API key");
    translationForm->addRow("API Key:", m_editApiKey);
    
    m_editTargetLang = new QLineEdit(translationGroup);
    m_editTargetLang->setPlaceholderText("e.g., th, en, ja");
    translationForm->addRow("Target Language:", m_editTargetLang);
    
    m_chkGoogleApi = new QCheckBox("Use Google Cloud Translation API", translationGroup);
    translationForm->addRow("", m_chkGoogleApi);
    
    settingsLayout->addWidget(translationGroup);
    
    // LLM Settings Group
    QGroupBox *llmGroup = new QGroupBox("LLM Translation", scrollContent);
    QFormLayout *llmForm = new QFormLayout(llmGroup);
    llmForm->setSpacing(Theme::Spacing::md());
    
    m_comboLlmProvider = new QComboBox(llmGroup);
    m_comboLlmProvider->addItem("None", "");
    m_comboLlmProvider->addItem("OpenAI", "openai");
    m_comboLlmProvider->addItem("Anthropic", "anthropic");
    m_comboLlmProvider->addItem("Custom", "custom");
    llmForm->addRow("Provider:", m_comboLlmProvider);
    
    m_editLlmApiKey = new QLineEdit(llmGroup);
    m_editLlmApiKey->setEchoMode(QLineEdit::Password);
    m_editLlmApiKey->setPlaceholderText("Enter your LLM API key");
    llmForm->addRow("API Key:", m_editLlmApiKey);
    
    m_editLlmModel = new QLineEdit(llmGroup);
    m_editLlmModel->setPlaceholderText("e.g., gpt-4, claude-3-sonnet");
    llmForm->addRow("Model:", m_editLlmModel);
    
    m_editLlmBaseUrl = new QLineEdit(llmGroup);
    m_editLlmBaseUrl->setPlaceholderText("Optional: Custom API base URL");
    llmForm->addRow("Base URL:", m_editLlmBaseUrl);
    
    settingsLayout->addWidget(llmGroup);
    
    settingsLayout->addStretch();
    
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);
    
    // Save button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton *btnSave = new QPushButton("Save Settings", this);
    connect(btnSave, &QPushButton::clicked, this, [this]() {
        saveSettings();
        emit settingsChanged();
    });
    buttonLayout->addWidget(btnSave);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect change signals
    connect(m_editApiKey, &QLineEdit::textChanged, this, &SettingsPage::settingsChanged);
    connect(m_editTargetLang, &QLineEdit::textChanged, this, &SettingsPage::settingsChanged);
    connect(m_chkGoogleApi, &QCheckBox::toggled, this, &SettingsPage::settingsChanged);
    connect(m_comboLlmProvider, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &SettingsPage::settingsChanged);
    connect(m_editLlmApiKey, &QLineEdit::textChanged, this, &SettingsPage::settingsChanged);
    connect(m_editLlmModel, &QLineEdit::textChanged, this, &SettingsPage::settingsChanged);
    connect(m_editLlmBaseUrl, &QLineEdit::textChanged, this, &SettingsPage::settingsChanged);
}

void SettingsPage::loadSettings()
{
    QSettings settings("NST", "NST");
    
    m_editApiKey->setText(settings.value("Translation/ApiKey").toString());
    m_editTargetLang->setText(settings.value("Translation/TargetLanguage", "th").toString());
    m_chkGoogleApi->setChecked(settings.value("Translation/UseGoogleApi", false).toBool());
    
    QString provider = settings.value("LLM/Provider").toString();
    int providerIndex = m_comboLlmProvider->findData(provider);
    if (providerIndex >= 0) {
        m_comboLlmProvider->setCurrentIndex(providerIndex);
    }
    
    m_editLlmApiKey->setText(settings.value("LLM/ApiKey").toString());
    m_editLlmModel->setText(settings.value("LLM/Model").toString());
    m_editLlmBaseUrl->setText(settings.value("LLM/BaseUrl").toString());
}

void SettingsPage::saveSettings()
{
    QSettings settings("NST", "NST");
    
    settings.setValue("Translation/ApiKey", m_editApiKey->text());
    settings.setValue("Translation/TargetLanguage", m_editTargetLang->text());
    settings.setValue("Translation/UseGoogleApi", m_chkGoogleApi->isChecked());
    
    settings.setValue("LLM/Provider", m_comboLlmProvider->currentData().toString());
    settings.setValue("LLM/ApiKey", m_editLlmApiKey->text());
    settings.setValue("LLM/Model", m_editLlmModel->text());
    settings.setValue("LLM/BaseUrl", m_editLlmBaseUrl->text());
}

QString SettingsPage::apiKey() const { return m_editApiKey->text(); }
QString SettingsPage::targetLanguage() const { return m_editTargetLang->text(); }
bool SettingsPage::useGoogleApi() const { return m_chkGoogleApi->isChecked(); }
QString SettingsPage::llmProvider() const { return m_comboLlmProvider->currentData().toString(); }
QString SettingsPage::llmApiKey() const { return m_editLlmApiKey->text(); }
QString SettingsPage::llmModel() const { return m_editLlmModel->text(); }
QString SettingsPage::llmBaseUrl() const { return m_editLlmBaseUrl->text(); }

} // namespace nst::ui
