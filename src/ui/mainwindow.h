#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QSharedPointer>

#include "menubar.h"
#include "customtitlebar.h"
#include "filetranslationwidget.h"
#include "settingsdialog.h"
#include "realtimetranslationwidget.h"
#include "imagetranslationwidget.h"
#include "updatecontroller.h"
#include "translationservicemanager.h"
#include "fontmanagerdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // CLI Support: Open project directly without dialog
    // deployAfterLoad: if true, deploy game after loading
    // outputPath: target directory for deployment (empty = in-place)
    // backupPreference: -1 = use settings, 0 = no backup, 1 = force backup
    void openProjectFromCLI(const QString &engineName, const QString &projectPath, 
                            bool deployAfterLoad = false, 
                            const QString &outputPath = QString(), 
                            int backupPreference = -1);

public slots:
    void onNewProject(); // Renamed

private slots:
// Restore missing slots
    void onOpenMockData();
    void onSettingsActionTriggered();
    void onFontsLoaded(const QJsonArray &fonts);
    void onFontManagerActionTriggered();
    void onPluginManagerActionTriggered();
    void onFeatureManagerActionTriggered();
    void onEditEngineScript(); // Renamed from onEditRpgmScript

    // View slots (delegated)
    void onToggleContext(bool checked);
    void onHideCompleted(bool checked);

    // Navigation slots
    void onNavigationChanged(int index);
    
    // Smart Filter slots (delegated)
    void onExportSmartFilterRules();
    void onImportSmartFilterRules();

    void onSaveProject();    // Renamed
    void onDeployProject();  // Renamed

private:
    void loadSettings();
    void saveSettings();
    void updateChildSettings();

private:
    Ui::MainWindow *ui;
    
    // UI Components
    MenuBar *m_menuBar;
    CustomTitleBar *m_titleBar;
    QStackedWidget *m_stackedWidget;
    
    // Widgets
    FileTranslationWidget *m_fileTranslationWidget;
    RealTimeTranslationWidget *m_realTimeWidget;
    ImageTranslationWidget *m_imageTranslationWidget;
    
    // Managers / Controllers owned by MainWindow but shared/used by children
    TranslationServiceManager *m_translationServiceManager;
    UpdateController *m_updateController;
    
    // Settings
    QString m_apiKey;
    QString m_targetLanguage;
    QString m_targetLanguageName;
    bool m_googleApi;
    QString m_engineName;   // Cached engine name for visibility usage
    QString m_llmProvider;
    QString m_llmApiKey;
    QString m_llmModel;
    QString m_llmBaseUrl;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    // Resize and Move handling
    enum ResizeDirection {
        ResizeNone = 0,
        ResizeTop = 1,
        ResizeBottom = 2,
        ResizeLeft = 4,
        ResizeRight = 8,
        ResizeTopLeft = 5,
        ResizeTopRight = 9,
        ResizeBottomLeft = 6,
        ResizeBottomRight = 10
    };
    int m_resizeDirection = ResizeNone;
    QPoint m_dragPosition;
    QRect m_originalGeometry;
    bool m_isDragging = false;
    
    void updateCursorShape(const QPoint &pos);
    int getResizeDirection(const QPoint &pos);
};

#endif // MAINWINDOW_H