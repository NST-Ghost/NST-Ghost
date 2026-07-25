#include "realtimetranslationwidget.h"
#include "processselectordialog.h"
#include <QDateTime>

RealTimeTranslationWidget::RealTimeTranslationWidget(QWidget *parent)
    : QWidget(parent), m_server(new TranslationServer(this)), m_isServerRunning(false), m_targetPid(-1)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    // Header / Controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    
    QLabel *portLabel = new QLabel("Port:", this);
    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(1024, 65535);
    m_portSpinBox->setValue(14478);
    m_portSpinBox->setFixedWidth(80);
    
    m_toggleServerButton = new QPushButton("Start Translation Server", this);
    m_toggleServerButton->setCheckable(true);
    m_toggleServerButton->setFixedHeight(40);
    
    m_statusLabel = new QLabel("Server Status: Stopped", this);
    m_statusLabel->setObjectName("realtimeStatusIdle");
    
    controlsLayout->addWidget(portLabel);
    controlsLayout->addWidget(m_portSpinBox);
    controlsLayout->addSpacing(10);
    controlsLayout->addWidget(m_toggleServerButton);
    
    m_selectProcessButton = new QPushButton("Select Game Process", this);
    m_selectProcessButton->setFixedHeight(40);
    controlsLayout->addSpacing(10);
    controlsLayout->addWidget(m_selectProcessButton);
    
    controlsLayout->addWidget(m_statusLabel);
    controlsLayout->addStretch();
    
    layout->addLayout(controlsLayout);
    
    // Log Viewer
    QLabel *logLabel = new QLabel("Server Logs / Incoming Text:", this);
    layout->addWidget(logLabel);
    
    m_logViewer = new QTextEdit(this);
    m_logViewer->setReadOnly(true);
    m_logViewer->setObjectName("realtimeLogViewer");
    layout->addWidget(m_logViewer);

    // Connections
    connect(m_toggleServerButton, &QPushButton::clicked, this, &RealTimeTranslationWidget::onToggleServer);
    connect(m_selectProcessButton, &QPushButton::clicked, this, &RealTimeTranslationWidget::onSelectProcess);
    connect(m_server, &TranslationServer::logMessage, this, &RealTimeTranslationWidget::onLogMessage);
    connect(m_server, &TranslationServer::newTranslationRequest, this, &RealTimeTranslationWidget::onNewTranslationRequest);
}

RealTimeTranslationWidget::~RealTimeTranslationWidget()
{
    if (m_server) {
        m_server->stopServer();
    }
}

void RealTimeTranslationWidget::onToggleServer()
{
    if (m_isServerRunning) {
        m_server->stopServer();
        m_toggleServerButton->setText("Start Translation Server");
        m_statusLabel->setText("Server Status: Stopped");
        m_statusLabel->setObjectName("realtimeStatusIdle");
        m_statusLabel->style()->unpolish(m_statusLabel);
        m_statusLabel->style()->polish(m_statusLabel);
        m_portSpinBox->setEnabled(true);
        m_isServerRunning = false;
    } else {
        int port = m_portSpinBox->value();
        if (m_server->startServer(port)) {
            m_toggleServerButton->setText("Stop Translation Server");
            m_statusLabel->setText(QString("Server Status: Listening on port %1").arg(port));
            m_statusLabel->setObjectName("realtimeStatusActive");
            m_statusLabel->style()->unpolish(m_statusLabel);
            m_statusLabel->style()->polish(m_statusLabel);
            m_portSpinBox->setEnabled(false);
            m_isServerRunning = true;
        } else {
            m_toggleServerButton->setChecked(false); // Reset button if failed
        }
    }
}

void RealTimeTranslationWidget::onSelectProcess()
{
    ProcessSelectorDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        m_targetPid = dialog.selectedPid();
        QString name = dialog.selectedName();
        onLogMessage(QString("Target Process Selected: %1 (PID: %2)").arg(name).arg(m_targetPid));
        
        // TODO: Trigger actual injection strategy here
        onLogMessage("Ready to inject... (Logic pending)");
    }
}

void RealTimeTranslationWidget::onLogMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logViewer->append(QString("[%1] %2").arg(timestamp, message));
}

void RealTimeTranslationWidget::onNewTranslationRequest(const QString &sourceText, QTcpSocket *socket)
{
    onLogMessage(QString("Received: \"%1\"").arg(sourceText));
    
    // TODO: Connect to Translation Service here.
    // For now, just echo back or send a dummy translation.
    
    QString dummyTranslation = "[TR] " + sourceText;
    if (socket && socket->isOpen()) {
         socket->write(dummyTranslation.toUtf8());
         socket->flush();
         onLogMessage(QString("Sent: \"%1\"").arg(dummyTranslation));
    }
}
