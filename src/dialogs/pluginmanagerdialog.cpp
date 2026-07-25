#include "pluginmanagerdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <QSplitter>
#include <QTime>

#ifdef HAS_LUA
#include "plugins/LuaScriptManager.h"
#endif

PluginManagerDialog::PluginManagerDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Plugin Manager");
    resize(700, 500);
    
    auto* layout = new QVBoxLayout(this);
    auto* splitter = new QSplitter(Qt::Vertical);
    
    // Top section
    auto* topWidget = new QWidget;
    auto* topLayout = new QVBoxLayout(topWidget);
    
    topLayout->addWidget(new QLabel("Available Plugins:"));
    m_pluginList = new QListWidget;
    topLayout->addWidget(m_pluginList);
    
    // Plugin info
    topLayout->addWidget(new QLabel("Plugin Info:"));
    m_infoText = new QTextEdit;
    m_infoText->setReadOnly(true);
    m_infoText->setMaximumHeight(80);
    topLayout->addWidget(m_infoText);
    
    // Enable checkbox
    m_enableCheckBox = new QCheckBox("Enable this plugin on startup");
    topLayout->addWidget(m_enableCheckBox);
    
    splitter->addWidget(topWidget);
    
    // Log output
    auto* logWidget = new QWidget;
    auto* logLayout = new QVBoxLayout(logWidget);
    logLayout->addWidget(new QLabel("Output Log:"));
    m_logOutput = new QTextEdit;
    m_logOutput->setReadOnly(true);
    m_logOutput->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: monospace;");
    logLayout->addWidget(m_logOutput);
    splitter->addWidget(logWidget);
    
    layout->addWidget(splitter);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout;
    m_installBtn = new QPushButton("Install Dependencies");
    m_runActionBtn = new QPushButton("Run Action");
    auto* reloadBtn = new QPushButton("Reload Plugins");
    auto* closeBtn = new QPushButton("Close");
    
    btnLayout->addWidget(m_installBtn);
    btnLayout->addWidget(m_runActionBtn);
    btnLayout->addWidget(reloadBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);
    
    connect(m_pluginList, &QListWidget::currentRowChanged, this, &PluginManagerDialog::onPluginSelected);
    connect(m_installBtn, &QPushButton::clicked, this, &PluginManagerDialog::onInstallClicked);
    connect(m_runActionBtn, &QPushButton::clicked, this, &PluginManagerDialog::onRunActionClicked);
    connect(reloadBtn, &QPushButton::clicked, this, &PluginManagerDialog::onReloadClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_enableCheckBox, &QCheckBox::checkStateChanged, this, &PluginManagerDialog::onEnableToggled);
    
#ifdef HAS_LUA
    // Connect Lua log output
    connect(&LuaScriptManager::instance(), &LuaScriptManager::logMessage, 
            this, &PluginManagerDialog::appendLog);
#endif
    
    loadPlugins();
    appendLog("Plugin Manager initialized");
}

void PluginManagerDialog::loadPlugins() {
    m_pluginList->clear();
    
#ifdef HAS_LUA
    QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts";
    QDir scriptDir(scriptPath);
    if (!scriptDir.exists()) {
        scriptDir.mkpath(".");
    }
    
    for (const QString& file : scriptDir.entryList({"*.lua"}, QDir::Files)) {
        m_pluginList->addItem(file);
    }
    
    if (m_pluginList->count() == 0) {
        m_pluginList->addItem("(No plugins found in " + scriptPath + ")");
        m_installBtn->setEnabled(false);
        m_runActionBtn->setEnabled(false);
    }
#else
    m_pluginList->addItem("(Lua support not available)");
    m_installBtn->setEnabled(false);
    m_runActionBtn->setEnabled(false);
#endif
}

void PluginManagerDialog::appendLog(const QString& msg) {
    m_logOutput->append("[" + QTime::currentTime().toString("HH:mm:ss") + "] " + msg);
}

QString PluginManagerDialog::getPluginStatus(const QString& pluginName) {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
    bool installed = settings.value("Plugins/" + pluginName + "/Installed", false).toBool();
    bool enabled = settings.value("Plugins/" + pluginName + "/Enabled", false).toBool();
    
    QString status;
    if (installed) status += "[OK] Installed";
    else status += "[NO] Not installed";
    
    if (enabled) status += " | [OK] Enabled";
    else status += " | [OFF] Disabled";
    
    return status;
}

void PluginManagerDialog::onPluginSelected() {
    auto* item = m_pluginList->currentItem();
    if (!item) return;
    
    QString pluginName = item->text();
    QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts";
    
    m_infoText->setText(
        "Plugin: " + pluginName + "\n" +
        "Location: " + scriptPath + "/" + pluginName + "\n" +
        "Status: " + getPluginStatus(pluginName)
    );
    
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
    bool enabled = settings.value("Plugins/" + pluginName + "/Enabled", false).toBool();
    m_enableCheckBox->setChecked(enabled);
    
    appendLog("Selected plugin: " + pluginName);
}

void PluginManagerDialog::onEnableToggled(int state) {
    auto* item = m_pluginList->currentItem();
    if (!item) return;
    
    QString pluginName = item->text();
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
    settings.setValue("Plugins/" + pluginName + "/Enabled", state == Qt::Checked);
    
    if (state == Qt::Checked) {
        appendLog("[OK] Enabled: " + pluginName + " (will load on next startup)");
    } else {
        appendLog("[OFF] Disabled: " + pluginName);
    }
    
    onPluginSelected(); // Refresh info
}

void PluginManagerDialog::onInstallClicked() {
#ifdef HAS_LUA
    auto* item = m_pluginList->currentItem();
    if (!item) return;
    
    QString pluginName = item->text();
    appendLog("=== Installing: " + pluginName + " ===");
    
    // Check if on_install hook exists
    if (!LuaScriptManager::instance().hasHook(pluginName, "on_install")) {
        // No installation hook needed, treat as success
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
        settings.setValue("Plugins/" + pluginName + "/Installed", true);
        appendLog("[OK] Installation completed (No dependencies)");
        onPluginSelected();
        return;
    }

    auto result = LuaScriptManager::instance().executeHookForPlugin(pluginName, "on_install");
    
    if (result.toBool()) {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
        settings.setValue("Plugins/" + pluginName + "/Installed", true);
        appendLog("[OK] Installation completed successfully");
    } else {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NST", "PluginSettings");
        settings.setValue("Plugins/" + pluginName + "/Installed", false);
        settings.setValue("Plugins/" + pluginName + "/Enabled", false); // Disable if install failed
        appendLog("[FAIL] Installation failed");
    }
    
    onPluginSelected(); // Refresh status
#endif
}

void PluginManagerDialog::onRunActionClicked() {
#ifdef HAS_LUA
    auto* item = m_pluginList->currentItem();
    if (!item) return;
    
    QString pluginName = item->text();
    appendLog("=== Running: " + pluginName + " ===");
    
    LuaScriptManager::instance().executeHookForPlugin(pluginName, "on_menu_click");
    
    appendLog("[OK] Action completed");
#endif
}

void PluginManagerDialog::onReloadClicked() {
#ifdef HAS_LUA
    appendLog("Reloading plugins...");
    QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts";
    LuaScriptManager::instance().loadScriptsFromDir(scriptPath);
    LuaScriptManager::instance().registerAPI();
    loadPlugins();
    appendLog("[OK] Plugins reloaded successfully");
#endif
}
