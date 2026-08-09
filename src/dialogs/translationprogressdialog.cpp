#include "translationprogressdialog.h"

// -------------------------------------------------------------
// PulseDotWidget Implementation
// -------------------------------------------------------------
PulseDotWidget::PulseDotWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(14, 14);
    connect(&m_animTimer, &QTimer::timeout, this, [this]() {
        if (m_state == Active) {
            if (m_fading) {
                m_alphaPulse -= 12;
                if (m_alphaPulse <= 80) { m_alphaPulse = 80; m_fading = false; }
            } else {
                m_alphaPulse += 12;
                if (m_alphaPulse >= 255) { m_alphaPulse = 255; m_fading = true; }
            }
        } else {
            m_alphaPulse = 255;
        }
        update();
    });
    m_animTimer.start(30); // ~33 FPS pulse
}

void PulseDotWidget::setState(State state)
{
    m_state = state;
    if (m_state == Active && !m_animTimer.isActive()) {
        m_animTimer.start(30);
    } else if (m_state != Active) {
        m_animTimer.stop();
        m_alphaPulse = 255;
    }
    update();
}

void PulseDotWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor color;
    switch (m_state) {
    case Active:  color = QColor(0, 255, 170, m_alphaPulse); break; // Glowing Cyan
    case Paused:  color = QColor(255, 170, 0, m_alphaPulse); break; // Amber
    case Success: color = QColor(0, 230, 118, 255); break;          // Emerald
    case Error:   color = QColor(255, 68, 68, 255); break;           // Crimson
    }

    // Outer subtle glow halo
    if (m_state == Active || m_state == Success) {
        QColor haloColor = color;
        haloColor.setAlpha(qMin(120, m_alphaPulse / 2));
        painter.setBrush(haloColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, width(), height());
    }

    // Inner vibrant core
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(2, 2, width() - 4, height() - 4);
}

// -------------------------------------------------------------
// LoadingSpinnerWidget Implementation
// -------------------------------------------------------------
LoadingSpinnerWidget::LoadingSpinnerWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(22, 22);
    setAttribute(Qt::WA_TranslucentBackground);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        m_angle = (m_angle + 10) % 360;
        update();
    });
}

void LoadingSpinnerWidget::start()
{
    m_running = true;
    if (!m_timer.isActive()) m_timer.start(16); // ~60 FPS
    show();
}

void LoadingSpinnerWidget::stop()
{
    m_running = false;
    m_timer.stop();
    update();
}

void LoadingSpinnerWidget::paintEvent(QPaintEvent *)
{
    if (!m_running) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    painter.setViewport((width() - side) / 2, (height() - side) / 2, side, side);
    painter.setWindow(-20, -20, 40, 40);

    // Track
    QPen trackPen(QColor(255, 255, 255, 25), 4);
    trackPen.setCapStyle(Qt::RoundCap);
    painter.setPen(trackPen);
    painter.drawEllipse(-15, -15, 30, 30);

    // Rotating Arc
    painter.rotate(m_angle);
    QConicalGradient gradient(0, 0, 0);
    gradient.setColorAt(0.0, QColor("#00ffaa"));
    gradient.setColorAt(0.5, QColor("#0088ff"));
    gradient.setColorAt(1.0, QColor(0, 255, 170, 0));

    QPen arcPen(QBrush(gradient), 4);
    arcPen.setCapStyle(Qt::RoundCap);
    painter.setPen(arcPen);
    painter.drawArc(-15, -15, 30, 30, 0, 240 * 16);
}

// -------------------------------------------------------------
// TranslationProgressDialog Implementation
// -------------------------------------------------------------
TranslationProgressDialog::TranslationProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Translation Progress Log Console");
    resize(880, 580);
    setModal(false); // Non-modal: Allows interacting with main window concurrently!

    // Modern Dark Glass Console Style with Shimmer & Glow
    setStyleSheet(
        "QDialog {"
        "   background-color: #16171d;"
        "   color: #e0e0e0;"
        "   font-family: 'Segoe UI', Arial, sans-serif;"
        "}"
        "QTextEdit {"
        "   background-color: #0d0e12;"
        "   color: #00ffaa;"
        "   font-family: 'JetBrains Mono', 'Consolas', 'Courier New', monospace;"
        "   font-size: 13px;"
        "   border: 1px solid #2a2d3a;"
        "   border-radius: 8px;"
        "   padding: 10px;"
        "}"
        "QProgressBar {"
        "   border: 1px solid #333748;"
        "   border-radius: 6px;"
        "   text-align: center;"
        "   background-color: #1e202a;"
        "   color: #ffffff;"
        "   font-weight: bold;"
        "   height: 20px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0088ff, stop:0.5 #00ffaa, stop:1 #00e676);"
        "   border-radius: 5px;"
        "}"
        "QPushButton {"
        "   background-color: #252836;"
        "   color: #ffffff;"
        "   border: 1px solid #3d4257;"
        "   border-radius: 6px;"
        "   padding: 8px 20px;"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #32374a;"
        "   border-color: #00ffaa;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #1a1c26;"
        "}"
        "QPushButton#bgButton {"
        "   background-color: #1b3835;"
        "   color: #00ffaa;"
        "   border: 1px solid #00aa88;"
        "}"
        "QPushButton#bgButton:hover {"
        "   background-color: #234c48;"
        "   border-color: #00ffaa;"
        "}"
        "QLabel {"
        "   color: #cccccc;"
        "   font-size: 13px;"
        "}"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(12);

    // Console Log Display
    m_consoleLog = new QTextEdit(this);
    m_consoleLog->setReadOnly(true);
    mainLayout->addWidget(m_consoleLog, 1);

    // Header Status Bar Layout with PulseDot and Spinner
    QHBoxLayout *statusHeaderLayout = new QHBoxLayout();
    statusHeaderLayout->setSpacing(8);

    m_pulseDot = new PulseDotWidget(this);
    m_spinner = new LoadingSpinnerWidget(this);

    m_statusLabel = new QLabel("Initializing translation pipeline...", this);
    m_statusLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #ffffff;");

    statusHeaderLayout->addWidget(m_pulseDot);
    statusHeaderLayout->addWidget(m_spinner);
    statusHeaderLayout->addWidget(m_statusLabel, 1);
    mainLayout->addLayout(statusHeaderLayout);

    // Stats Info Label (Elapsed Time | Speed | ETA)
    m_statsLabel = new QLabel("Elapsed: 00:00  |  Speed: 0.0 items/s  |  ETA: --:--", this);
    m_statsLabel->setStyleSheet("color: #8a8f9e; font-family: 'JetBrains Mono', 'Consolas', monospace; font-size: 12px;");
    m_statsLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statsLabel);

    // Progress Bar Layout
    QHBoxLayout *progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    m_percentLabel = new QLabel("0%", this);
    m_percentLabel->setFixedWidth(55);
    m_percentLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_percentLabel->setStyleSheet("font-weight: bold; color: #00ffaa; font-size: 14px;");

    progressLayout->addWidget(m_progressBar, 1);
    progressLayout->addWidget(m_percentLabel);
    mainLayout->addLayout(progressLayout);

    // UX Hint Label
    m_tipLabel = new QLabel("💡 Tip: You can hide or close this window to work on other projects. Click status bar at bottom anytime to re-open.", this);
    m_tipLabel->setStyleSheet("color: #6c7285; font-size: 11px; font-style: italic;");
    m_tipLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_tipLabel);

    // Button Controls Layout (Abort / Pause / Run in Background)
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_backgroundButton = new QPushButton("Run in Background 🔽", this);
    m_backgroundButton->setObjectName("bgButton");
    m_pauseButton = new QPushButton("Pause", this);
    m_abortButton = new QPushButton("Abort", this);

    buttonLayout->addWidget(m_backgroundButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_abortButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    connect(m_backgroundButton, &QPushButton::clicked, this, &TranslationProgressDialog::onBackgroundButtonClicked);
    connect(m_abortButton, &QPushButton::clicked, this, &TranslationProgressDialog::onAbortButtonClicked);
    connect(m_pauseButton, &QPushButton::clicked, this, &TranslationProgressDialog::onPauseButtonClicked);

    // Connect Stats Update Ticker (fires every 1 second)
    connect(&m_ticker, &QTimer::timeout, this, &TranslationProgressDialog::updateStatsTimer);
}

void TranslationProgressDialog::appendLog(const QString &message)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString escapedMsg = message.toHtmlEscaped();

    QString color = "#00ffaa"; // Default bright cyan/green
    if (message.contains("[ERROR]") || message.contains("[HTTP ERROR]") || message.contains("[LUA ERROR]")) {
        color = "#ff5555"; // Bright Red/Pink
    } else if (message.contains("[WARN]") || message.contains("[RETRY]")) {
        color = "#ffb86c"; // Orange/Yellow
    } else if (message.contains("[HTTP SEND]")) {
        color = "#8be9fd"; // Soft Blue/Cyan
    } else if (message.contains("[HTTP RECV]") || message.contains("[SUCCESS]") || message.contains("[OK]")) {
        color = "#50fa7b"; // Bright Emerald Green
    } else if (message.contains("[WORKER-")) {
        color = "#bd93f9"; // Soft Purple/Violet
    }

    QString html = QString("<span style='color:#6272a4;'>[%1]</span> <span style='color:%2;'>%3</span>")
                       .arg(timeStr, color, escapedMsg);
    m_consoleLog->append(html);
    m_consoleLog->ensureCursorVisible();
}

void TranslationProgressDialog::setProgress(int current, int total)
{
    if (total <= 0) return;
    m_currentItemCount = current;
    m_totalItemCount = total;

    int percent = (current * 100) / total;
    m_progressBar->setValue(percent);
    m_percentLabel->setText(QString("%1%").arg(percent));
    appendLog(QString("Batch progress: %1 / %2 (%3%)").arg(current).arg(total).arg(percent));

    if (current >= total) {
        m_isJobRunning = false;
        m_pulseDot->setState(PulseDotWidget::Success);
        m_spinner->stop();
        m_ticker.stop();
        setStatusText("Translation Finished!");
    }

    updateStatsTimer();
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

    m_isJobRunning = true;
    m_jobTimer.restart();
    m_ticker.start(1000); // 1-second interval
    m_pulseDot->setState(PulseDotWidget::Active);
    m_spinner->start();
}

void TranslationProgressDialog::reset()
{
    m_consoleLog->clear();
    m_progressBar->setValue(0);
    m_percentLabel->setText("0%");
    m_statusLabel->setText("Ready");
    m_statsLabel->setText("Elapsed: 00:00  |  Speed: 0.0 items/s  |  ETA: --:--");
    m_isPaused = false;
    m_isJobRunning = false;
    m_pauseButton->setText("Pause");
    m_currentItemCount = 0;
    m_totalItemCount = 0;
    m_pulseDot->setState(PulseDotWidget::Active);
    m_spinner->stop();
    m_ticker.stop();
}

void TranslationProgressDialog::updateStatsTimer()
{
    if (!m_jobTimer.isValid()) return;

    qint64 elapsedSec = m_jobTimer.elapsed() / 1000;
    int mins = elapsedSec / 60;
    int secs = elapsedSec % 60;
    QString elapsedStr = QString("%1:%2")
                             .arg(mins, 2, 10, QChar('0'))
                             .arg(secs, 2, 10, QChar('0'));

    double speed = (elapsedSec > 0) ? static_cast<double>(m_currentItemCount) / elapsedSec : 0.0;
    
    QString etaStr = "--:--";
    if (speed > 0.001 && m_totalItemCount > m_currentItemCount) {
        int remainingItems = m_totalItemCount - m_currentItemCount;
        int remainingSec = static_cast<int>(remainingItems / speed);
        int etaMins = remainingSec / 60;
        int etaSecs = remainingSec % 60;
        etaStr = QString("%1:%2")
                     .arg(etaMins, 2, 10, QChar('0'))
                     .arg(etaSecs, 2, 10, QChar('0'));
    }

    m_statsLabel->setText(QString("⏱️ Elapsed: %1  |  ⚡ Speed: %2 items/s  |  ⏳ ETA: %3")
                              .arg(elapsedStr)
                              .arg(speed, 0, 'f', 1)
                              .arg(etaStr));
}

void TranslationProgressDialog::onBackgroundButtonClicked()
{
    hide();
}

void TranslationProgressDialog::closeEvent(QCloseEvent *event)
{
    if (m_isJobRunning) {
        // If job is actively running, closing the window just hides it so translation isn't lost!
        event->ignore();
        hide();
    } else {
        QDialog::closeEvent(event);
    }
}

void TranslationProgressDialog::onPauseButtonClicked()
{
    m_isPaused = !m_isPaused;
    if (m_isPaused) {
        m_pauseButton->setText("Resume");
        appendLog("[INFO] Translation PAUSED by user.");
        setStatusText("Translation Paused");
        m_pulseDot->setState(PulseDotWidget::Paused);
        m_spinner->stop();
    } else {
        m_pauseButton->setText("Pause");
        appendLog("[INFO] Translation RESUMED.");
        setStatusText("Translating...");
        m_pulseDot->setState(PulseDotWidget::Active);
        m_spinner->start();
    }
    emit pauseToggled(m_isPaused);
}

void TranslationProgressDialog::onAbortButtonClicked()
{
    m_isJobRunning = false;
    appendLog("[WARN] Translation ABORTED by user.");
    m_pulseDot->setState(PulseDotWidget::Error);
    m_spinner->stop();
    m_ticker.stop();
    emit canceled();
    close();
}
