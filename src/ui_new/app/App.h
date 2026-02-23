#ifndef NST_UI_APP_H
#define NST_UI_APP_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPointer>

// Forward declarations
class TranslationServiceManager;
class UpdateController;

namespace nst::ui {

class AppNavigation;
class FileTranslationPage;
class SettingsPage;

/**
 * @brief Main application window shell.
 * 
 * Manages navigation between pages and owns shared services.
 * This is the main entry point for the new UI architecture.
 */
class App : public QMainWindow
{
    Q_OBJECT

public:
    enum class Page {
        FileTranslation = 0,
        Settings
    };

    explicit App(QWidget *parent = nullptr);
    ~App() override;

    // Navigate to a specific page
    void navigateTo(Page page);

    // Access shared services
    TranslationServiceManager* translationService() const { return m_translationService; }

public slots:
    void onOpenProject(const QString &engineName, const QString &projectPath);

signals:
    void pageChanged(Page page);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();
    void setupNavigation();
    void setupPages();
    void connectSignals();
    void loadSettings();
    void saveSettings();
    void applyTheme();

private:
    // Navigation
    AppNavigation *m_navigation = nullptr;
    QStackedWidget *m_pageStack = nullptr;

    // Pages
    FileTranslationPage *m_fileTranslationPage = nullptr;
    SettingsPage *m_settingsPage = nullptr;

    // Shared Services (owned by App)
    TranslationServiceManager *m_translationService = nullptr;
    UpdateController *m_updateController = nullptr;

    // Settings cache
    struct Settings {
        QString apiKey;
        QString targetLanguage;
        bool useGoogleApi = false;
        QString llmProvider;
        QString llmApiKey;
        QString llmModel;
        QString llmBaseUrl;
    } m_settings;
};

} // namespace nst::ui

#endif // NST_UI_APP_H
