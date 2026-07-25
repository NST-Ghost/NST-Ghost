#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "plugindebuggerdialog.h"
#include "pluginmanagerdialog.h"
#include "pluginmanagerdialog.h"
#include "loadprojectdialog.h"
#include "scripteditordialog.h"
#include "featuremanagerdialog.h"
#include <qtlingo/translationsettings.h>

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
#include <QFileDialog>
#include <QHBoxLayout>
#include <QTabBar>
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

    // Initialize global status BEFORE children widget creation
    m_globalStatusWidget = new GlobalStatusWidget(this);
    statusBar()->addPermanentWidget(m_globalStatusWidget);
    
    connect(m_globalStatusWidget, &GlobalStatusWidget::tableStatusClicked, this, [this]() {
        if (m_projectTabBar->count() > 0 && m_projectTabBar->currentIndex() != -1) {
            onProjectTabChanged(m_projectTabBar->currentIndex());
        }
    });

    loadSettings();

    // Use default Qt window (no custom frameless handling)
    setWindowTitle("NST Translation Tool");
    
    // Setup Tab Bar (styled like Chrome tabs)
    m_projectTabBar = new QTabBar(this);
    m_projectTabBar->setTabsClosable(true);
    m_projectTabBar->setMovable(true);
    m_projectTabBar->setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    m_projectTabBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    connect(m_projectTabBar, &QTabBar::currentChanged, this, &MainWindow::onProjectTabChanged);
    connect(m_projectTabBar, &QTabBar::tabCloseRequested, this, &MainWindow::onProjectTabCloseRequested);

    m_addProjectButton = new QPushButton("+", this);
    m_addProjectButton->setObjectName("addProjectButton");
    m_addProjectButton->setFixedSize(24, 24);
    m_addProjectButton->setCursor(Qt::PointingHandCursor);
    connect(m_addProjectButton, &QPushButton::clicked, this, &MainWindow::onAddProjectButtonClicked);

    QHBoxLayout *tabBarLayout = new QHBoxLayout();
    tabBarLayout->setContentsMargins(6, 4, 6, 0);
    tabBarLayout->setSpacing(6);
    tabBarLayout->addWidget(m_projectTabBar);
    tabBarLayout->addWidget(m_addProjectButton);
    tabBarLayout->addStretch();

    // Setup MenuBar
    m_menuBar = new MenuBar(this);

    // Create Stacked Widget and Pages
    m_stackedWidget = new QStackedWidget(this);

    // Create a new container widget
    QWidget *mainContainer = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Add widgets to layout
    m_menuBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_stackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLayout->addLayout(tabBarLayout);
    mainLayout->addWidget(m_menuBar);
    mainLayout->addWidget(m_stackedWidget);
    
    mainLayout->setStretch(0, 0); // Tab Bar Layout
    mainLayout->setStretch(1, 0); // Menu Bar
    mainLayout->setStretch(2, 1); // Content
    
    setCentralWidget(mainContainer);
    
    // Set minimum size
    setMinimumSize(800, 600);

    // Connect MenuBar signals to MainWindow slots
    connect(m_menuBar, &MenuBar::openMockData, this, &MainWindow::onOpenMockData);
    connect(m_menuBar, &MenuBar::newProject, this, &MainWindow::onNewProject);
    connect(m_menuBar, &MenuBar::updateProject, this, &MainWindow::onUpdateProject);
    connect(m_menuBar, &MenuBar::openProject, this, &MainWindow::onOpenProject);
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
    delete m_updateController;
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
        if (m_projectTabBar->count() == 0) {
            emit returnToProjectManager();
        }
        return;
    }

    QString engineName = dialog.selectedEngine();
    m_engineName = engineName;
    QString projectPath = dialog.projectPath();

    if (engineName.isEmpty() || projectPath.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please select both engine and project path.");
        if (m_projectTabBar->count() == 0) {
            emit returnToProjectManager();
        }
        return;
    }
    
    addNewProjectTab(engineName, projectPath);
}

void MainWindow::loadProjectFile(const QString &nstFilePath)
{
    if (nstFilePath.isEmpty()) return;
    
    // Check if the project is already open
    for (int i = 0; i < m_projectTabBar->count(); ++i) {
        int pageIndex = m_projectTabBar->tabData(i).toInt();
        if (auto *proj = qobject_cast<FileTranslationWidget*>(m_stackedWidget->widget(pageIndex))) {
            if (proj->currentProjectFile() == nstFilePath) {
                m_projectTabBar->setCurrentIndex(i);
                m_stackedWidget->setCurrentIndex(pageIndex);
                return;
            }
        }
    }
    
    // Create new tab/project
    FileTranslationWidget *projWidget = new FileTranslationWidget(m_translationServiceManager, this);
    projWidget->setSettings(m_apiKey, m_targetLanguage, m_googleApi, 
                            m_llmProvider, m_llmApiKey, m_llmModel, m_llmBaseUrl,
                            m_sourceLanguage);
    
    connect(projWidget, &FileTranslationWidget::translationStateChanged,
            m_globalStatusWidget, &GlobalStatusWidget::setTableTranslationActive);
    connect(projWidget, &FileTranslationWidget::taskFinished, this, &MainWindow::taskFinished);
            
    connect(projWidget, &FileTranslationWidget::projectLoaded, this, [this, projWidget](const QString &projectPath) {
        int pageIdx = m_stackedWidget->indexOf(projWidget);
        for (int i = 0; i < m_projectTabBar->count(); ++i) {
            if (m_projectTabBar->tabData(i).toInt() == pageIdx) {
                QString name;
                QString file = projWidget->currentProjectFile();
                if (!file.isEmpty()) {
                    name = QFileInfo(file).fileName();
                } else {
                    name = QDir(projectPath).dirName();
                }
                if (name.isEmpty()) name = "Project";
                m_projectTabBar->setTabText(i, name);
                break;
            }
        }
    });

    int pageIndex = m_stackedWidget->addWidget(projWidget);
    
    QString name = QFileInfo(nstFilePath).fileName();
    if (name.isEmpty()) name = "Project";
    
    int tabIndex = m_projectTabBar->addTab(name);
    m_projectTabBar->setTabData(tabIndex, pageIndex);
    
    // Switch to it
    m_projectTabBar->setCurrentIndex(tabIndex);
    m_stackedWidget->setCurrentIndex(pageIndex);
    
    projWidget->loadProjectFile(nstFilePath);
}

void MainWindow::onReturnToProjectManager()
{
    // Save all projects before leaving if modified
    for (int i = 0; i < m_projectTabBar->count(); ++i) {
        int pageIndex = m_projectTabBar->tabData(i).toInt();
        if (auto *proj = qobject_cast<FileTranslationWidget*>(m_stackedWidget->widget(pageIndex))) {
            proj->onSaveProject();
        }
    }
    emit returnToProjectManager();
}

void MainWindow::onOpenMockData()
{
    if (auto *proj = currentProjectWidget()) {
        proj->openMockData();
    }
}

void MainWindow::openProjectFromCLI(const QString &engineName, const QString &projectPath,
                                     bool deployAfterLoad, bool translateAfterLoad,
                                     const QString &outputPath, 
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

    // Open project in tab
    FileTranslationWidget *projWidget = addNewProjectTab(matchedEngine, projectPath, true);
    
    // Set default deployment path from CLI if provided (so GUI deploy button won't ask)
    if (!outputPath.isEmpty()) {
         projWidget->setDefaultDeploymentPath(outputPath);
    }

    // Handle translate after load if requested
    if (translateAfterLoad) {
        std::cerr << "[NST] CLI Translate: queued..." << std::endl;
        QTimer::singleShot(1000, this, [projWidget]() {
            projWidget->onTranslateAllCLI();
        });
    }

    // Handle deploy after load if requested
    if (deployAfterLoad) {
        // Determine final output path (empty = in-place deployment)
        QString targetDir = outputPath.isEmpty() ? projectPath : outputPath;
        
        // Determine backup setting
        bool shouldBackup;
        if (backupPreference == -1) {
            QSettings settings;
            shouldBackup = settings.value("deployBackupEnabled", true).toBool();
        } else {
            shouldBackup = (backupPreference == 1);
        }
        
        std::cerr << "[NST] CLI Deploy: target=" << targetDir.toStdString() 
                  << ", backup=" << (shouldBackup ? "yes" : "no") << std::endl;
        
        // Schedule deploy after project is fully loaded
        int delay = translateAfterLoad ? 5000 : 1500; 
        
        QTimer::singleShot(delay, this, [projWidget, targetDir, shouldBackup]() {
            projWidget->onDeployProjectCLI(targetDir, shouldBackup);
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
    dialog.setSourceLanguage(m_sourceLanguage);
    
    // Backup setting
    TranslationSettings tSettings;
    tSettings.load();
    dialog.setBackupEnabled(tSettings.deployBackupEnabled);
    
    // AI Filter
    if (auto *proj = currentProjectWidget()) {
        dialog.setAiFilterEnabled(proj->isAiFilterEnabled());
        dialog.setAiFilterThreshold(proj->aiFilterThreshold());
    }
  
    // Set current mode
    dialog.setTranslationMode(m_translationMode);
  
    if (dialog.exec() == QDialog::Accepted) {
        m_apiKey = dialog.googleApiKey();
        m_targetLanguage = dialog.targetLanguage();
        m_targetLanguageName = dialog.targetLanguageName();
        m_googleApi = dialog.isGoogleApi();
        m_llmProvider = dialog.llmProvider();
        m_llmApiKey = dialog.llmApiKey();
        m_llmModel = dialog.llmModel();
        m_llmBaseUrl = dialog.llmBaseUrl();
        m_sourceLanguage = dialog.sourceLanguage();
        
        // Capture mode
        m_translationMode = dialog.translationMode();
        
        // Backup setting
        tSettings.deployBackupEnabled = dialog.isBackupEnabled();
        
        // AI Filter
        if (auto *proj = currentProjectWidget()) {
            proj->setAiFilterEnabled(dialog.isAiFilterEnabled());
            proj->setAiFilterThreshold(dialog.aiFilterThreshold());
        }
  
        // Save using unified settings structure
        tSettings.googleApiKey = m_apiKey;
        tSettings.targetLanguage = m_targetLanguage;
        tSettings.googleApiEnabled = m_googleApi;
        tSettings.llmProvider = m_llmProvider;
        tSettings.llmApiKey = m_llmApiKey;
        tSettings.llmModel = m_llmModel;
        tSettings.llmBaseUrl = m_llmBaseUrl;
        tSettings.sourceLanguage = m_sourceLanguage;
        tSettings.translationMode = m_translationMode;
        tSettings.save();

        updateChildSettings();
    }
}

void MainWindow::loadSettings()
{
     TranslationSettings tSettings;
     tSettings.load();
     m_apiKey = tSettings.googleApiKey;
     m_targetLanguage = tSettings.targetLanguage;
     m_googleApi = tSettings.googleApiEnabled;
     m_llmProvider = tSettings.llmProvider;
     m_llmApiKey = tSettings.llmApiKey;
     m_llmModel = tSettings.llmModel;
     m_llmBaseUrl = tSettings.llmBaseUrl;
     m_sourceLanguage = tSettings.sourceLanguage;
     m_translationMode = tSettings.translationMode;
}

void MainWindow::saveSettings()
{
     TranslationSettings tSettings;
     tSettings.googleApiKey = m_apiKey;
     tSettings.targetLanguage = m_targetLanguage;
     tSettings.googleApiEnabled = m_googleApi;
     tSettings.llmProvider = m_llmProvider;
     tSettings.llmApiKey = m_llmApiKey;
     tSettings.llmModel = m_llmModel;
     tSettings.llmBaseUrl = m_llmBaseUrl;
     tSettings.sourceLanguage = m_sourceLanguage;
     tSettings.translationMode = m_translationMode;
     tSettings.save();
}

void MainWindow::updateChildSettings()
{
    for (int i = 0; i < m_projectTabBar->count(); ++i) {
        int pageIndex = m_projectTabBar->tabData(i).toInt();
        if (auto *proj = qobject_cast<FileTranslationWidget*>(m_stackedWidget->widget(pageIndex))) {
            proj->setSettings(m_apiKey, m_targetLanguage, m_googleApi,
                              m_llmProvider, m_llmApiKey, m_llmModel, m_llmBaseUrl,
                              m_sourceLanguage);
        }
    }
}

void MainWindow::onFontsLoaded(const QJsonArray &fonts) {}

void MainWindow::onFontManagerActionTriggered()
{
    if (auto *proj = currentProjectWidget()) {
        proj->openFontManager();
    }
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

void MainWindow::onToggleContext(bool checked) { if (auto *proj = currentProjectWidget()) proj->onToggleContext(checked); }
void MainWindow::onHideCompleted(bool checked) { if (auto *proj = currentProjectWidget()) proj->onHideCompleted(checked); }
void MainWindow::onExportSmartFilterRules() { if (auto *proj = currentProjectWidget()) proj->onExportSmartFilterRules(); }
void MainWindow::onImportSmartFilterRules() { if (auto *proj = currentProjectWidget()) proj->onImportSmartFilterRules(); }
void MainWindow::onSaveProject() { if (auto *proj = currentProjectWidget()) proj->onSaveProject(); }
void MainWindow::onDeployProject() { if (auto *proj = currentProjectWidget()) proj->onDeployProject(); }

void MainWindow::onEditEngineScript()
{
    auto *proj = currentProjectWidget();
    if (!proj || !proj->getProjectDataManager() || !proj->getBGADataManager()) {
        QMessageBox::warning(this, "Error", "Data manager not available.");
        return;
    }

    QString projectPath = proj->getProjectDataManager()->getProjectPath();
    QString engineName = proj->getProjectDataManager()->getEngineName();

    if (projectPath.isEmpty()) {
        QMessageBox::warning(this, "No Project", "No project is currently loaded.");
        return;
    }
    
    QPair<QString, QString> scriptDetails = proj->getBGADataManager()->getScriptDetails(engineName, projectPath);
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

void MainWindow::onOpenProject()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Project"), 
                                                    "", 
                                                    tr("NST Workspace Files (*.nst)"));
    if (filePath.isEmpty()) return;
    loadProjectFile(filePath);
}

void MainWindow::onUpdateProject()
{
    if (auto *proj = currentProjectWidget()) {
        proj->onUpdateProject();
    }
}

void MainWindow::onAddProjectButtonClicked()
{
    onNewProject();
}



void MainWindow::onProjectTabChanged(int index)
{
    if (index < 0 || index >= m_projectTabBar->count()) return;
    
    int pageIndex = m_projectTabBar->tabData(index).toInt();
    m_stackedWidget->setCurrentIndex(pageIndex);
    
    // Update global status widget for the current active tab
    if (auto *proj = qobject_cast<FileTranslationWidget*>(m_stackedWidget->widget(pageIndex))) {
        m_globalStatusWidget->setTableTranslationActive(proj->isTranslating());
    } else {
        m_globalStatusWidget->setTableTranslationActive(false);
    }
}

void MainWindow::onProjectTabCloseRequested(int index)
{
    if (index < 0 || index >= m_projectTabBar->count()) return;
    
    int pageIndex = m_projectTabBar->tabData(index).toInt();
    QWidget *widget = m_stackedWidget->widget(pageIndex);
    
    if (auto *proj = qobject_cast<FileTranslationWidget*>(widget)) {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, tr("Close Project"),
            tr("Are you sure you want to close this project?\nMake sure you have saved your changes."),
            QMessageBox::No | QMessageBox::Yes,
            QMessageBox::Yes);
        if (resBtn != QMessageBox::Yes) {
            return;
        }
        
        m_projectTabBar->removeTab(index);
        m_stackedWidget->removeWidget(proj);
        delete proj;
        
        // Adjust subsequent tabData values since removal shifts remaining indices
        for (int i = 0; i < m_projectTabBar->count(); ++i) {
            int oldIdx = m_projectTabBar->tabData(i).toInt();
            if (oldIdx > pageIndex) {
                m_projectTabBar->setTabData(i, oldIdx - 1);
            }
        }
    }
    
    // If no tabs are left, go back to project manager
    if (m_projectTabBar->count() == 0) {
        emit returnToProjectManager();
    }
}

FileTranslationWidget* MainWindow::currentProjectWidget() const
{
    return qobject_cast<FileTranslationWidget*>(m_stackedWidget->currentWidget());
}

FileTranslationWidget* MainWindow::addNewProjectTab(const QString &engineName, const QString &projectPath, bool cliMode)
{
    FileTranslationWidget *projWidget = new FileTranslationWidget(m_translationServiceManager, this);
    projWidget->setSettings(m_apiKey, m_targetLanguage, m_googleApi, 
                            m_llmProvider, m_llmApiKey, m_llmModel, m_llmBaseUrl,
                            m_sourceLanguage);
    
    connect(projWidget, &FileTranslationWidget::translationStateChanged,
            m_globalStatusWidget, &GlobalStatusWidget::setTableTranslationActive);
    connect(projWidget, &FileTranslationWidget::taskFinished, this, &MainWindow::taskFinished);
            
    connect(projWidget, &FileTranslationWidget::projectLoaded, this, [this, projWidget](const QString &projectPath) {
        int pageIdx = m_stackedWidget->indexOf(projWidget);
        for (int i = 0; i < m_projectTabBar->count(); ++i) {
            if (m_projectTabBar->tabData(i).toInt() == pageIdx) {
                QString name;
                QString file = projWidget->currentProjectFile();
                if (!file.isEmpty()) {
                    name = QFileInfo(file).fileName();
                } else {
                    name = QDir(projectPath).dirName();
                }
                if (name.isEmpty()) name = "Project";
                m_projectTabBar->setTabText(i, name);
                break;
            }
        }
    });

    int pageIndex = m_stackedWidget->addWidget(projWidget);
    
    QString folderName = QDir(projectPath).dirName();
    if (folderName.isEmpty()) {
        folderName = "New Project";
    }
    
    int tabIndex = m_projectTabBar->addTab(folderName);
    m_projectTabBar->setTabData(tabIndex, pageIndex);
    
    m_projectTabBar->setCurrentIndex(tabIndex);
    m_stackedWidget->setCurrentIndex(pageIndex);
    
    projWidget->onNewProject(engineName, projectPath, cliMode);
    
    return projWidget;
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
    QMainWindow::mouseDoubleClickEvent(event);
}
