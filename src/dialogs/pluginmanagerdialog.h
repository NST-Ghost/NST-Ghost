#pragma once
#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QCheckBox>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>

class PluginManagerDialog : public QDialog {
    Q_OBJECT
public:
    explicit PluginManagerDialog(QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onPluginSelected();
    void onInstallClicked();
    void onImportClicked();
    void onRunActionClicked();
    void onReloadClicked();
    void onEnableToggled(int state);

private:
    QListWidget* m_pluginList;
    QTextEdit* m_logOutput;
    QTextEdit* m_infoText;
    QPushButton* m_installBtn;
    QPushButton* m_importBtn;
    QPushButton* m_runActionBtn;
    QCheckBox* m_enableCheckBox;
    
    void loadPlugins();
    void appendLog(const QString& msg);
    QString getPluginStatus(const QString& pluginName);
    bool importPluginFile(const QString& filePath);
};
