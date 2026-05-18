#include "menubar.h"
#include <QMenu>

MenuBar::MenuBar(QWidget *parent)
    : QMenuBar(parent)
{
    createActions();
    createMenus();
}

void MenuBar::createMenus()
{
    fileMenu = addMenu(tr("&File"));
    fileMenu->addAction(newProjectAction);
    fileMenu->addAction(openProjectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(saveProjectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(deployProjectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(returnToProjectManagerAction); // Added
    fileMenu->addSeparator();
    fileMenu->addAction(settingsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    viewMenu = addMenu(tr("&View"));
    viewMenu->addAction(toggleContextAction);
    viewMenu->addAction(hideCompletedAction);

    toolsMenu = addMenu(tr("&Tools"));
    toolsMenu->addAction(fontManagerAction);
    toolsMenu->addAction(pluginManagerAction);
    toolsMenu->addAction(featureManagerAction);
    toolsMenu->addAction(editEngineScriptAction); // Renamed

    smartFilterMenu = addMenu(tr("&Smart Filter"));
    smartFilterMenu->addAction(importSmartFilterRulesAction);
    smartFilterMenu->addAction(exportSmartFilterRulesAction);
}

void MenuBar::createActions()
{
    openMockDataAction = new QAction(tr("Open Mock Data"), this);
    connect(openMockDataAction, &QAction::triggered, this, &MenuBar::openMockData);

    newProjectAction = new QAction(tr("New Project..."), this);
    connect(newProjectAction, &QAction::triggered, this, &MenuBar::newProject);

    openProjectAction = new QAction(tr("Open Project..."), this);
    connect(openProjectAction, &QAction::triggered, this, &MenuBar::openProject);

    saveProjectAction = new QAction(tr("Save"), this);
    saveProjectAction->setShortcut(QKeySequence::Save);
    connect(saveProjectAction, &QAction::triggered, this, &MenuBar::saveProject);

    deployProjectAction = new QAction(tr("Deploy Game..."), this);
    connect(deployProjectAction, &QAction::triggered, this, &MenuBar::deployProject);

    returnToProjectManagerAction = new QAction(tr("Project Manager..."), this);
    returnToProjectManagerAction->setShortcut(QKeySequence("Shift+1"));
    connect(returnToProjectManagerAction, &QAction::triggered, this, &MenuBar::returnToProjectManager);

    settingsAction = new QAction(tr("Settings"), this);
    connect(settingsAction, &QAction::triggered, this, &MenuBar::settings);

    exitAction = new QAction(tr("&Exit"), this);
    connect(exitAction, &QAction::triggered, this, &MenuBar::exit);

    fontManagerAction = new QAction(tr("Font Manager..."), this);
    connect(fontManagerAction, &QAction::triggered, this, &MenuBar::fontManager);

    pluginManagerAction = new QAction(tr("Plugin Manager..."), this);
    connect(pluginManagerAction, &QAction::triggered, this, &MenuBar::pluginManager);

    featureManagerAction = new QAction(tr("Feature Manager..."), this);
    connect(featureManagerAction, &QAction::triggered, this, &MenuBar::featureManager);

    editEngineScriptAction = new QAction(tr("Edit Engine Script..."), this);
    connect(editEngineScriptAction, &QAction::triggered, this, &MenuBar::editEngineScript);

    // View Actions
    toggleContextAction = new QAction(tr("Show Context Column"), this);
    toggleContextAction->setCheckable(true);
    toggleContextAction->setChecked(true);
    connect(toggleContextAction, &QAction::toggled, this, &MenuBar::toggleContext);

    hideCompletedAction = new QAction(tr("Hide Completed Rows"), this);
    hideCompletedAction->setCheckable(true);
    hideCompletedAction->setChecked(false);
    connect(hideCompletedAction, &QAction::toggled, this, &MenuBar::hideCompleted);

    // Smart Filter Actions
    exportSmartFilterRulesAction = new QAction(tr("Export Rules..."), this);
    connect(exportSmartFilterRulesAction, &QAction::triggered, this, &MenuBar::exportSmartFilterRules);

    importSmartFilterRulesAction = new QAction(tr("Import Rules..."), this);
    connect(importSmartFilterRulesAction, &QAction::triggered, this, &MenuBar::importSmartFilterRules);
}
