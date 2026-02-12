#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "plugindebuggerdialog.h"
#include "pluginmanagerdialog.h"
#include "pluginmanagerdialog.h"
#include "loadprojectdialog.h"
#include "scripteditordialog.h"
#include "featuremanagerdialog.h"

#ifdef HAS_LUA
#include "plugins/LuaScriptManager.h"
#endif

#include <QStandardPaths>
#include <QStyle>
#include <QApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QString logFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/mainwindow_log.txt";
    QFile logFile(logFilePath);
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/icon-app.png"));
    resize(1024, 768);
    
    // Remove the default menu bar created by ui->setupUi
    if (ui->menubar) {
        delete ui->menubar;
        ui->menubar = nullptr;
    }

    // Initialize Managers
    m_translationServiceManager = new TranslationServiceManager(this);
    connect(m_translationServiceManager, &TranslationServiceManager::errorOccurred, this, [this](const QString &message){
       statusBar()->showMessage("Translation Service Error: " + message, 5000); 
    });

    loadSettings();

    // Use default Qt window (no custom frameless handling)
    setWindowTitle("NST Translation Tool");
    
    // Create Custom Title Bar
    m_titleBar = new CustomTitleBar(this);
    m_titleBar->setTitle("NST Translation Tool");
    m_titleBar->setIcon(QIcon(":/icons/icon-app.png"));
    
    connect(m_titleBar, &CustomTitleBar::minimizeClicked, this, &QMainWindow::showMinimized);
    connect(m_titleBar, &CustomTitleBar::maximizeRestoreClicked, this, [this]() {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(m_titleBar, &CustomTitleBar::closeClicked, this, &QMainWindow::close);

    // Setup MenuBar
    m_menuBar = new MenuBar(this);

    // Create Stacked Widget and Pages
    m_stackedWidget = new QStackedWidget(this);
    
    // Page 0: File Translation Widget
    m_fileTranslationWidget = new FileTranslationWidget(m_translationServiceManager, this);
    m_fileTranslationWidget->setSettings(m_apiKey, m_targetLanguage, m_googleApi, 
                                         m_llmProvider, m_llmApiKey, m_llmModel, m_llmBaseUrl);
    m_stackedWidget->addWidget(m_fileTranslationWidget); 
    
    // Page 1: Real-time Translation Widget
    m_realTimeWidget = new RealTimeTranslationWidget(this);
    m_stackedWidget->addWidget(m_realTimeWidget);

    // Page 2: Image Translation Widget
    m_imageTranslationWidget = new ImageTranslationWidget(m_translationServiceManager, this);
    m_imageTranslationWidget->setSettings(m_apiKey, m_targetLanguage, m_googleApi, 
                                          m_llmProvider, m_llmApiKey, m_llmModel, m_llmBaseUrl);
    m_stackedWidget->addWidget(m_imageTranslationWidget);
    // Create a new container widget
    QWidget *mainContainer = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(0);
    
    // Add widgets to layout
    m_titleBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_menuBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_stackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLayout->addWidget(m_titleBar);
    mainLayout->addWidget(m_menuBar);
    mainLayout->addWidget(m_stackedWidget);
    
    mainLayout->setStretch(0, 0); // Title Bar
    mainLayout->setStretch(1, 0); // Menu Bar
    mainLayout->setStretch(2, 1); // Content
    
    setCentralWidget(mainContainer);
    
    connect(m_titleBar, &CustomTitleBar::translateModeClicked, this, [this]() {
        onNavigationChanged(0);
    });
    connect(m_titleBar, &CustomTitleBar::realTimeModeClicked, this, [this]() {
        onNavigationChanged(1);
    });
    connect(m_titleBar, &CustomTitleBar::imageTranslationClicked, this, [this]() {
        onNavigationChanged(2);
    });
    
    // Set minimum size
    setMinimumSize(800, 600);

    // Connect MenuBar signals to FileTranslationWidget slots
    connect(m_menuBar, &MenuBar::openMockData, m_fileTranslationWidget, &FileTranslationWidget::openMockData);
    connect(m_menuBar, &MenuBar::newProject, this, &MainWindow::onNewProject);
    connect(m_menuBar, &MenuBar::openProject, m_fileTranslationWidget, &FileTranslationWidget::onOpenProject);
    
    // Changing strategy: Connect to FileTranslationWidget signal if it exists, or add one.
    // Let's assume for now I need to check FileTranslationWidget first.
    connect(m_menuBar, &MenuBar::settings, this, &MainWindow::onSettingsActionTriggered);
    connect(m_menuBar, &MenuBar::saveProject, this, &MainWindow::onSaveProject);
    connect(m_menuBar, &MenuBar::deployProject, this, &MainWindow::onDeployProject);
    connect(m_menuBar, &MenuBar::exit, this, &QMainWindow::close);
    connect(m_menuBar, &MenuBar::fontManager, this, &MainWindow::onFontManagerActionTriggered);
    connect(m_menuBar, &MenuBar::pluginManager, this, &MainWindow::onPluginManagerActionTriggered);
    connect(m_menuBar, &MenuBar::featureManager, this, &MainWindow::onFeatureManagerActionTriggered);
    connect(m_menuBar, &MenuBar::editEngineScript, this, &MainWindow::onEditEngineScript); // Renamed
    connect(m_menuBar, &MenuBar::toggleContext, this, &MainWindow::onToggleContext);
    connect(m_menuBar, &MenuBar::hideCompleted, this, &MainWindow::onHideCompleted);
    connect(m_menuBar, &MenuBar::exportSmartFilterRules, this, &MainWindow::onExportSmartFilterRules);
    connect(m_menuBar, &MenuBar::importSmartFilterRules, this, &MainWindow::onImportSmartFilterRules);

    m_updateController = new UpdateController(this);
    m_updateController->checkForUpdates();

#ifdef HAS_LUA
    // Load enabled Lua plugins only
    QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts";
    QDir scriptDir(scriptPath);
    QSettings settings;
    
    int loadedCount = 0;
    for (const QString& file : scriptDir.entryList({"*.lua"}, QDir::Files)) {
        bool enabled = settings.value("plugins/" + file + "/enabled", false).toBool();
        if (enabled) {
            LuaScriptManager::instance().loadScriptsFromDir(scriptPath);
            LuaScriptManager::instance().registerAPI();
            loadedCount++;
        }
    }
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onNewProject()
{
    BGADataManager tempManager(this); 
    QStringList availableEngines = tempManager.getAvailableAnalyzers();
    
    if (availableEngines.isEmpty()) {
        QMessageBox::warning(this, "Error", "No game analyzers available.");
        return;
    }

    LoadProjectDialog dialog(availableEngines, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString engineName = dialog.selectedEngine();
    // Update member
    m_engineName = engineName;
    QString projectPath = dialog.projectPath();

    if (engineName.isEmpty() || projectPath.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please select both engine and project path.");
        return;
    }
    
    m_fileTranslationWidget->onNewProject(engineName, projectPath);
}

void MainWindow::onOpenMockData()
{
    if (m_fileTranslationWidget) {
        m_fileTranslationWidget->openMockData();
    }
}

void MainWindow::openProjectFromCLI(const QString &engineName, const QString &projectPath,
                                     bool deployAfterLoad, const QString &outputPath, 
                                     int backupPreference)
{
    if (engineName.isEmpty() || projectPath.isEmpty()) {
        qWarning() << "[NST] CLI Error: Both --engine and --project are required";
        return;
    }
    
    // Validate engine exists
    BGADataManager tempManager(this);
    QStringList availableEngines = tempManager.getAvailableAnalyzers();
    
    // Case-insensitive engine matching
    QString matchedEngine;
    for (const QString &engine : availableEngines) {
        if (engine.compare(engineName, Qt::CaseInsensitive) == 0) {
            matchedEngine = engine;
            break;
        }
    }
    
    if (matchedEngine.isEmpty()) {
        QMessageBox::warning(this, "Invalid Engine", 
            QString("Engine '%1' not found.\n\nAvailable engines:\n%2")
            .arg(engineName, availableEngines.join("\n")));
        return;
    }
    
    // Validate project path exists
    if (!QDir(projectPath).exists()) {
        QMessageBox::warning(this, "Invalid Path",
            QString("Project path does not exist:\n%1").arg(projectPath));
        return;
    }

    // Open project (cliMode=true for auto-save without dialog)
    m_engineName = matchedEngine;
    m_fileTranslationWidget->onNewProject(matchedEngine, projectPath, true);
    
    // Set default deployment path from CLI if provided (so GUI deploy button won't ask)
    if (!outputPath.isEmpty()) {
         m_fileTranslationWidget->setDefaultDeploymentPath(outputPath);
    } else {
         // Default to project path if not specified? 
         // User requested "set beforehand". If not set, maybe don't force it to allow selecting?
         // But for consistency with CLI deploy behavior (in-place), maybe we should?
         // Let's only set it if explicitly provided via --output for now, 
         // OR if deployAfterLoad is true (since that implies we know where to go).
         // Actually, let's keep it optional. If user wants to skip dialog, they use -o or -d.
    }

    // Handle deploy after load if requested
    if (deployAfterLoad) {
        // Determine final output path (empty = in-place deployment)
        QString targetDir = outputPath.isEmpty() ? projectPath : outputPath;
        
        // Determine backup setting
        // backupPreference: -1 = use settings, 0 = no backup, 1 = force backup
        bool shouldBackup;
        if (backupPreference == -1) {
            // Use setting from QSettings
            QSettings settings;
            shouldBackup = settings.value("deployBackupEnabled", true).toBool();
        } else {
            shouldBackup = (backupPreference == 1);
        }
        
        std::cerr << "[NST] CLI Deploy: target=" << targetDir.toStdString() 
                  << ", backup=" << (shouldBackup ? "yes" : "no") << std::endl;
        
        // Schedule deploy after project is fully loaded
        // Use a delayed singleShot to give time for project loading
        QTimer::singleShot(500, this, [this, targetDir, shouldBackup]() {
            m_fileTranslationWidget->onDeployProjectCLI(targetDir, shouldBackup);
        });
    }
}

void MainWindow::onSettingsActionTriggered()
{
    SettingsDialog dialog(this);
    dialog.setGoogleApiKey(m_apiKey);
    dialog.setTargetLanguage(m_targetLanguage);
    dialog.setGoogleApi(m_googleApi);
    dialog.setLlmProvider(m_llmProvider);
    dialog.setLlmApiKey(m_llmApiKey);
    dialog.setLlmModel(m_llmModel);
    dialog.setLlmBaseUrl(m_llmBaseUrl);
    
    // Backup setting
    QSettings settings;
    dialog.setBackupEnabled(settings.value("deployBackupEnabled", true).toBool());
    
    // AI Filter
    if (m_fileTranslationWidget) {
        dialog.setAiFilterEnabled(m_fileTranslationWidget->isAiFilterEnabled());
        dialog.setAiFilterThreshold(m_fileTranslationWidget->aiFilterThreshold());
    }

    if (dialog.exec() == QDialog::Accepted) {
        m_apiKey = dialog.googleApiKey();
        m_targetLanguage = dialog.targetLanguage();
        m_targetLanguageName = dialog.targetLanguageName();
        m_googleApi = dialog.isGoogleApi();
        m_llmProvider = dialog.llmProvider();
        m_llmApiKey = dialog.llmApiKey();
        m_llmModel = dialog.llmModel();
        m_llmBaseUrl = dialog.llmBaseUrl();
        
        // Backup setting
        settings.setValue("deployBackupEnabled", dialog.isBackupEnabled());
        
        // AI Filter
        if (m_fileTranslationWidget) {
            m_fileTranslationWidget->setAiFilterEnabled(dialog.isAiFilterEnabled());
            m_fileTranslationWidget->setAiFilterThreshold(dialog.aiFilterThreshold());
        }

        saveSettings();
        updateChildSettings();
    }
}

void MainWindow::loadSettings()
{
     QSettings settings;
     m_apiKey = settings.value("googleApiKey").toString();
     m_targetLanguage = settings.value("targetLanguage", "th").toString();
     m_googleApi = settings.value("googleApi", false).toBool();
     m_llmProvider = settings.value("llmProvider").toString();
     m_llmApiKey = settings.value("llmApiKey").toString();
     m_llmApiKey = settings.value("llmApiKey").toString();
     m_llmModel = settings.value("llmModel").toString();
     m_llmBaseUrl = settings.value("llmBaseUrl").toString();
}

void MainWindow::saveSettings()
{
     QSettings settings;
     settings.setValue("googleApiKey", m_apiKey);
     settings.setValue("targetLanguage", m_targetLanguage);
     settings.setValue("googleApi", m_googleApi);
     settings.setValue("llmProvider", m_llmProvider);
     settings.setValue("llmApiKey", m_llmApiKey);
     settings.setValue("llmModel", m_llmModel);
     settings.setValue("llmBaseUrl", m_llmBaseUrl);
}

void MainWindow::updateChildSettings()
{
    if (m_fileTranslationWidget) {
        m_fileTranslationWidget->setSettings(m_apiKey, m_targetLanguage, m_googleApi,
                                             m_llmProvider, m_llmApiKey, m_llmModel, m_llmBaseUrl);
    }
    if (m_imageTranslationWidget) {
        m_imageTranslationWidget->setSettings(m_apiKey, m_targetLanguage, m_googleApi,
                                              m_llmProvider, m_llmApiKey, m_llmModel, m_llmBaseUrl);
    }
}

void MainWindow::onFontsLoaded(const QJsonArray &fonts) {}

void MainWindow::onFontManagerActionTriggered()
{
    m_fileTranslationWidget->openFontManager();
}


void MainWindow::onPluginManagerActionTriggered()
{
    PluginManagerDialog dialog(this);
    dialog.exec();
}

void MainWindow::onFeatureManagerActionTriggered()
{
    FeatureManagerDialog dialog(this);
    dialog.exec();
}

void MainWindow::onToggleContext(bool checked) { m_fileTranslationWidget->onToggleContext(checked); }
void MainWindow::onHideCompleted(bool checked) { m_fileTranslationWidget->onHideCompleted(checked); }
void MainWindow::onExportSmartFilterRules() { m_fileTranslationWidget->onExportSmartFilterRules(); }
void MainWindow::onImportSmartFilterRules() { m_fileTranslationWidget->onImportSmartFilterRules(); }
void MainWindow::onSaveProject() { m_fileTranslationWidget->onSaveProject(); }
void MainWindow::onDeployProject() { m_fileTranslationWidget->onDeployProject(); }

void MainWindow::onEditEngineScript()
{
    if (!m_fileTranslationWidget || !m_fileTranslationWidget->getProjectDataManager() || !m_fileTranslationWidget->getBGADataManager()) {
        QMessageBox::warning(this, "Error", "Data manager not available.");
        return;
    }

    QString projectPath = m_fileTranslationWidget->getProjectDataManager()->getProjectPath();
    QString engineName = m_fileTranslationWidget->getProjectDataManager()->getEngineName();

    if (projectPath.isEmpty()) {
        QMessageBox::warning(this, "No Project", "No project is currently loaded.");
        return;
    }
    
    QPair<QString, QString> scriptDetails = m_fileTranslationWidget->getBGADataManager()->getScriptDetails(engineName, projectPath);
    QString scriptPath = scriptDetails.first;
    QString targetFunction = scriptDetails.second;

    if (scriptPath.isEmpty()) {
         QMessageBox::information(this, "Not Supported", "Script editing not supported for engine: " + engineName);
         return;
    }

    QFile scriptFile(scriptPath);
    if (!scriptFile.exists()) {
        QMessageBox::warning(this, "File Not Found", "Could not find script file in the project directory.\nExpected at: " + scriptPath);
        return;
    }

    ScriptEditorDialog dialog(scriptPath, targetFunction, this);
    dialog.exec();
}

void MainWindow::onNavigationChanged(int index)
{
    m_stackedWidget->setCurrentIndex(index);
}

// Qt native window management - no custom mouse handling needed
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    QMainWindow::mouseMoveEvent(event);
}

// Qt handles cursor and resize direction natively
void MainWindow::updateCursorShape(const QPoint &pos)
{
    Q_UNUSED(pos);
}

int MainWindow::getResizeDirection(const QPoint &pos)
{
    Q_UNUSED(pos);
    return ResizeNone;
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_titleBar && m_titleBar->geometry().contains(event->pos())) {
         if (isMaximized()) showNormal();
         else showMaximized();
    }
    QMainWindow::mouseDoubleClickEvent(event);
}
