#include "translationprogressdialog.h"

TranslationProgressDialog::TranslationProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Translation Progress Log Console");
    resize(850, 550);
    setModal(true);

    // Modern Dark Glass Console Style
    setStyleSheet(
        "QDialog {"
        "   background-color: #1a1a1a;"
        "   color: #e0e0e0;"
        "   font-family: 'Segoe UI', Arial, sans-serif;"
        "}"
        "QTextEdit {"
        "   background-color: #111111;"
        "   color: #00ffaa;"
        "   font-family: 'Consolas', 'Courier New', monospace;"
        "   font-size: 13px;"
        "   border: 1px solid #333333;"
        "   border-radius: 6px;"
        "   padding: 10px;"
        "}"
        "QProgressBar {"
        "   border: 1px solid #444444;"
        "   border-radius: 4px;"
        "   text-align: center;"
        "   background-color: #222222;"
        "   color: #ffffff;"
        "   font-weight: bold;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00aa88, stop:1 #00ffaa);"
        "   border-radius: 3px;"
        "}"
        "QPushButton {"
        "   background-color: #2a2a2a;"
        "   color: #ffffff;"
        "   border: 1px solid #444444;"
        "   border-radius: 4px;"
        "   padding: 8px 24px;"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #3a3a3a;"
        "   border-color: #666666;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #1a1a1a;"
        "}"
        "QLabel {"
        "   color: #cccccc;"
        "   font-size: 13px;"
        "}"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Console Log Display
    m_consoleLog = new QTextEdit(this);
    m_consoleLog->setReadOnly(true);
    mainLayout->addWidget(m_consoleLog, 1);

    // Status Label
    m_statusLabel = new QLabel("Initializing translation pipeline...", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);

    // Progress Layout
    QHBoxLayout *progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    
    m_percentLabel = new QLabel("0%", this);
    m_percentLabel->setFixedWidth(50);
    m_percentLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    progressLayout->addWidget(m_progressBar, 1);
    progressLayout->addWidget(m_percentLabel);
    mainLayout->addLayout(progressLayout);

    // Button Controls Layout (Abort / Pause)
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_abortButton = new QPushButton("Abort", this);
    m_pauseButton = new QPushButton("Pause", this);

    buttonLayout->addWidget(m_abortButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    connect(m_abortButton, &QPushButton::clicked, this, &TranslationProgressDialog::onAbortButtonClicked);
    connect(m_pauseButton, &QPushButton::clicked, this, &TranslationProgressDialog::onPauseButtonClicked);
}

void TranslationProgressDialog::appendLog(const QString &message)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_consoleLog->append(QString("[%1] %2").arg(timeStr, message));
    m_consoleLog->ensureCursorVisible();
}

void TranslationProgressDialog::setProgress(int current, int total)
{
    if (total <= 0) return;
    int percent = (current * 100) / total;
    m_progressBar->setValue(percent);
    m_percentLabel->setText(QString("%1%").arg(percent));
    appendLog(QString("Batch progress: %1 / %2 (%3%)").arg(current).arg(total).arg(percent));
}

void TranslationProgressDialog::setStatusText(const QString &statusText)
{
    m_statusLabel->setText(statusText);
}

void TranslationProgressDialog::setOptionsHeader(const QString &serviceName, const QStringList &selectedFiles, const QJsonObject &options)
{
    m_consoleLog->clear();
    appendLog("=================================================");
    appendLog("[INFO] Translation Job Started!");
    appendLog("=================================================");
    appendLog("Translator Engine: " + serviceName);
    if (!selectedFiles.isEmpty()) {
        appendLog("Selected Files: " + selectedFiles.join(", "));
    }
    
    QJsonDocument doc(options);
    appendLog("Configuration Options:");
    appendLog(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
    appendLog("-------------------------------------------------");
}

void TranslationProgressDialog::reset()
{
    m_consoleLog->clear();
    m_progressBar->setValue(0);
    m_percentLabel->setText("0%");
    m_statusLabel->setText("Ready");
    m_isPaused = false;
    m_pauseButton->setText("Pause");
}

void TranslationProgressDialog::onPauseButtonClicked()
{
    m_isPaused = !m_isPaused;
    if (m_isPaused) {
        m_pauseButton->setText("Resume");
        appendLog("[INFO] Translation PAUSED by user.");
        setStatusText("Translation Paused");
    } else {
        m_pauseButton->setText("Pause");
        appendLog("[INFO] Translation RESUMED.");
        setStatusText("Translating...");
    }
    emit pauseToggled(m_isPaused);
}

void TranslationProgressDialog::onAbortButtonClicked()
{
    appendLog("[WARN] Translation ABORTED by user.");
    emit canceled();
    close();
}
