#ifndef NST_UI_SETTINGSPAGE_H
#define NST_UI_SETTINGSPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>

namespace nst::ui {

/**
 * @brief Settings page for application configuration.
 */
class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

    // Settings accessors
    QString apiKey() const;
    QString targetLanguage() const;
    bool useGoogleApi() const;
    QString llmProvider() const;
    QString llmApiKey() const;
    QString llmModel() const;
    QString llmBaseUrl() const;

signals:
    void settingsChanged();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

private:
    // Translation settings
    QLineEdit *m_editApiKey = nullptr;
    QLineEdit *m_editTargetLang = nullptr;
    QCheckBox *m_chkGoogleApi = nullptr;
    
    // LLM settings
    QComboBox *m_comboLlmProvider = nullptr;
    QLineEdit *m_editLlmApiKey = nullptr;
    QLineEdit *m_editLlmModel = nullptr;
    QLineEdit *m_editLlmBaseUrl = nullptr;
};

} // namespace nst::ui

#endif // NST_UI_SETTINGSPAGE_H
