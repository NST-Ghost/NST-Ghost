#ifndef TRANSLATIONPROGRESSDIALOG_H
#define TRANSLATIONPROGRESSDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

class TranslationProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TranslationProgressDialog(QWidget *parent = nullptr);
    ~TranslationProgressDialog() = default;

public slots:
    void appendLog(const QString &message);
    void setProgress(int current, int total);
    void setStatusText(const QString &statusText);
    void setOptionsHeader(const QString &serviceName, const QStringList &selectedFiles, const QJsonObject &options);
    void reset();

signals:
    void canceled();
    void pauseToggled(bool isPaused);

private slots:
    void onPauseButtonClicked();
    void onAbortButtonClicked();

private:
    QTextEdit *m_consoleLog;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_percentLabel;
    QPushButton *m_pauseButton;
    QPushButton *m_abortButton;
    bool m_isPaused = false;
};

#endif // TRANSLATIONPROGRESSDIALOG_H
