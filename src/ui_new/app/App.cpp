#include "App.h"
#include "AppNavigation.h"
#include "../pages/FileTranslationPage.h"
#include "../pages/ImageTranslationPage.h"
#include "../pages/SettingsPage.h"
#include "../styles/Theme.h"

#include "translationservicemanager.h"
#include "updatecontroller.h"

#include <QHBoxLayout>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>

namespace nst::ui {

App::App(QWidget *parent)
    : QMainWindow(parent)
{
    // Create shared services
    m_translationService = new TranslationServiceManager(this);
    m_updateController = new UpdateController(this);
    
    setupUI();
    connectSignals();
    loadSettings();
    applyTheme();
}

App::~App()
{
    saveSettings();
}

void App::setupUI()
{
    setWindowTitle("NST - Novel & Script Translator");
    setMinimumSize(1200, 800);
    
    // Central widget
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Navigation sidebar
    setupNavigation();
    mainLayout->addWidget(m_navigation);
    
    // Page stack
    m_pageStack = new QStackedWidget(central);
    mainLayout->addWidget(m_pageStack, 1);
    
    // Setup pages
    setupPages();
}

void App::setupNavigation()
{
    m_navigation = new AppNavigation(this);
}

void App::setupPages()
{
    // File Translation Page
    m_fileTranslationPage = new FileTranslationPage(m_translationService, this);
    m_pageStack->addWidget(m_fileTranslationPage);
    
    // Image Translation Page
    m_imageTranslationPage = new ImageTranslationPage(m_translationService, this);
    m_pageStack->addWidget(m_imageTranslationPage);
    
    // Settings Page
    m_settingsPage = new SettingsPage(this);
    m_pageStack->addWidget(m_settingsPage);
}

void App::connectSignals()
{
    // Navigation
    connect(m_navigation, &AppNavigation::navigationRequested,
            this, [this](int index) {
                navigateTo(static_cast<Page>(index));
            });
    
    // Settings changes
    connect(m_settingsPage, &SettingsPage::settingsChanged,
            this, [this]() {
                // Propagate settings to pages
                // TODO: implement settings propagation
            });
}

void App::navigateTo(Page page)
{
    int index = static_cast<int>(page);
    if (index >= 0 && index < m_pageStack->count()) {
        m_pageStack->setCurrentIndex(index);
        m_navigation->setActiveIndex(index);
        emit pageChanged(page);
    }
}

void App::onOpenProject(const QString &engineName, const QString &projectPath)
{
    // Navigate to file translation and open project
    navigateTo(Page::FileTranslation);
    m_fileTranslationPage->openProject(engineName, projectPath);
}

void App::loadSettings()
{
    QSettings settings("NST", "NST");
    
    m_settings.apiKey = settings.value("Translation/ApiKey").toString();
    m_settings.targetLanguage = settings.value("Translation/TargetLanguage", "th").toString();
    m_settings.useGoogleApi = settings.value("Translation/UseGoogleApi", false).toBool();
    m_settings.llmProvider = settings.value("LLM/Provider").toString();
    m_settings.llmApiKey = settings.value("LLM/ApiKey").toString();
    m_settings.llmModel = settings.value("LLM/Model").toString();
    m_settings.llmBaseUrl = settings.value("LLM/BaseUrl").toString();
    
    // Restore window geometry
    restoreGeometry(settings.value("Window/Geometry").toByteArray());
    restoreState(settings.value("Window/State").toByteArray());
}

void App::saveSettings()
{
    QSettings settings("NST", "NST");
    
    settings.setValue("Translation/ApiKey", m_settings.apiKey);
    settings.setValue("Translation/TargetLanguage", m_settings.targetLanguage);
    settings.setValue("Translation/UseGoogleApi", m_settings.useGoogleApi);
    settings.setValue("LLM/Provider", m_settings.llmProvider);
    settings.setValue("LLM/ApiKey", m_settings.llmApiKey);
    settings.setValue("LLM/Model", m_settings.llmModel);
    settings.setValue("LLM/BaseUrl", m_settings.llmBaseUrl);
    
    // Save window geometry
    settings.setValue("Window/Geometry", saveGeometry());
    settings.setValue("Window/State", saveState());
}

void App::applyTheme()
{
    qApp->setStyleSheet(Theme::loadStylesheet());
}

void App::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

} // namespace nst::ui
