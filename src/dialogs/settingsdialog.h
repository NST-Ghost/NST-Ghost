#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class QListWidgetItem;

QT_BEGIN_NAMESPACE
namespace Ui {
class SettingsDialog;
}
QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    QString googleApiKey() const;
    QString sourceLanguage() const;
    QString targetLanguage() const;
    QString targetLanguageName() const;
    bool isGoogleApi() const;
    QString llmProvider() const;
    QString llmApiKey() const;
    QString llmModel() const;
    QString llmBaseUrl() const;
    bool isRelationsEnabled() const;
    
    // AI Filter
    bool isAiFilterEnabled() const;
    double aiFilterThreshold() const;
    
    // Sidebar & Translation Mode
    int translationMode() const;
    void setTranslationMode(int mode);

    void setGoogleApiKey(const QString &apiKey);
    void setSourceLanguage(const QString &language);
    void setTargetLanguage(const QString &language);
    void setGoogleApi(bool isApi);
    void setLlmProvider(const QString &provider);
    void setLlmApiKey(const QString &apiKey);
    void setLlmModel(const QString &model);
    void setLlmBaseUrl(const QString &baseUrl);
    void setRelationsEnabled(bool enabled);
    void setAiFilterEnabled(bool enabled);
    void setAiFilterThreshold(double threshold);
    
    // Backup
    bool isBackupEnabled() const;
    void setBackupEnabled(bool enabled);

private slots:
    void updateLlmModelComboBox();
    void fetchLlmModels();
    void fetchPluginModels();
    void updateLlmModelVisibility(bool animate = true);
    void updatePluginModelVisibility(bool animate = true);
    void testConnection();
    void updateLuaPluginEngineUI(const QString &scriptName);

private:
    Ui::SettingsDialog *ui;
    QNetworkAccessManager *m_networkManager;
    
    void setupPluginsUI();
    void loadPluginList();
    void onPluginSelected(QListWidgetItem *item);
    QString parsePluginMetadata(const QString &scriptPath);
};

#endif // SETTINGSDIALOG_H
