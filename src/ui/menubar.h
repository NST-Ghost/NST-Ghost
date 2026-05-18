#ifndef MENUBAR_H
#define MENUBAR_H

#include <QMenuBar>

class MenuBar : public QMenuBar
{
    Q_OBJECT
public:
    explicit MenuBar(QWidget *parent = nullptr);

signals:
    void openMockData();
    void newProject();      // Renamed from loadFromGameProject
    void openProject();     // Renamed from openWorkspace
    void settings();
    void saveProject();     // Kept same name, logic changes
    void deployProject();   // Renamed from exportProject
    void exit();
    void returnToProjectManager(); // Added
    void fontManager();
    void pluginManager();
    void featureManager();  // Feature Manager dialog
    void editEngineScript(); // Renamed from editRpgmScript

    // View signals
    void toggleContext(bool checked);
    void hideCompleted(bool checked);

    // Smart Filter signals
    void exportSmartFilterRules();
    void importSmartFilterRules();

private:
    void createMenus();
    void createActions();

    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *toolsMenu;
    QMenu *smartFilterMenu;
    QAction *openMockDataAction;
    QAction *newProjectAction;      // Renamed
    QAction *openProjectAction;     // Renamed
    QAction *settingsAction;
    QAction *saveProjectAction;
    QAction *deployProjectAction;   // Renamed
    QAction *returnToProjectManagerAction; // Added
    QAction *exitAction;
    QAction *fontManagerAction;
    QAction *pluginManagerAction;
    QAction *featureManagerAction;
    QAction *editEngineScriptAction; // Renamed from editRpgmScriptAction

    // View actions
    QAction *toggleContextAction;
    QAction *hideCompletedAction;

    // Smart Filter actions
    QAction *exportSmartFilterRulesAction;
    QAction *importSmartFilterRulesAction;
};

#endif // MENUBAR_H