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
#include <QElapsedTimer>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QConicalGradient>
#include <QCloseEvent>

// Custom Glowing Pulse Dot Widget
class PulseDotWidget : public QWidget {
    Q_OBJECT
public:
    enum State { Active, Paused, Success, Error };

    explicit PulseDotWidget(QWidget *parent = nullptr);
    void setState(State state);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    State m_state = Active;
    QTimer m_animTimer;
    int m_alphaPulse = 255;
    bool m_fading = true;
};

// Custom Rotating Gradient Arc Spinner Widget
class LoadingSpinnerWidget : public QWidget {
    Q_OBJECT
public:
    explicit LoadingSpinnerWidget(QWidget *parent = nullptr);
    void start();
    void stop();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QTimer m_timer;
    int m_angle = 0;
    bool m_running = false;
};

class TranslationProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TranslationProgressDialog(QWidget *parent = nullptr);
    ~TranslationProgressDialog() = default;
    bool isJobRunning() const { return m_isJobRunning; }

public slots:
    void appendLog(const QString &message);
    void setProgress(int current, int total);
    void setStatusText(const QString &statusText);
    void setOptionsHeader(const QString &serviceName, const QStringList &selectedFiles, const QJsonObject &options);
    void reset();

signals:
    void canceled();
    void pauseToggled(bool isPaused);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPauseButtonClicked();
    void onAbortButtonClicked();
    void onBackgroundButtonClicked();
    void updateStatsTimer();

private:
    QTextEdit *m_consoleLog;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_percentLabel;
    QLabel *m_statsLabel;
    QLabel *m_tipLabel;
    QPushButton *m_pauseButton;
    QPushButton *m_abortButton;
    QPushButton *m_backgroundButton;
    
    PulseDotWidget *m_pulseDot;
    LoadingSpinnerWidget *m_spinner;

    QElapsedTimer m_jobTimer;
    QTimer m_ticker;
    int m_currentItemCount = 0;
    int m_totalItemCount = 0;
    bool m_isPaused = false;
    bool m_isJobRunning = false;
};

#endif // TRANSLATIONPROGRESSDIALOG_H
