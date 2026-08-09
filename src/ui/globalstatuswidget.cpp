#include "globalstatuswidget.h"
#include <QMouseEvent>
#include <QPalette>

GlobalStatusWidget::GlobalStatusWidget(QWidget *parent)
    : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    setToolTip("Click to open Translation Progress Console Log");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(8);

    // Table Translation Status Indicator Dot
    m_tableIndicator = new QLabel(this);
    m_tableIndicator->setFixedSize(10, 10);
    
    m_tableLabel = new QLabel("Text: Ready", this);
    m_tableLabel->setStyleSheet("font-weight: 500;");

    m_miniProgressBar = new QProgressBar(this);
    m_miniProgressBar->setFixedWidth(100);
    m_miniProgressBar->setFixedHeight(14);
    m_miniProgressBar->setRange(0, 100);
    m_miniProgressBar->setValue(0);
    m_miniProgressBar->setTextVisible(false);
    m_miniProgressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 1px solid #444;"
        "   border-radius: 3px;"
        "   background-color: #222;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0088ff, stop:1 #00ffaa);"
        "   border-radius: 2px;"
        "}"
    );
    m_miniProgressBar->hide();

    layout->addWidget(m_tableIndicator);
    layout->addWidget(m_tableLabel);
    layout->addWidget(m_miniProgressBar);
    layout->addStretch();

    setTableTranslationActive(false);
}

void GlobalStatusWidget::setTableTranslationActive(bool active)
{
    updateStatusIndicator(m_tableIndicator, active);
    if (!active) {
        m_tableLabel->setText("Text: Ready");
        m_miniProgressBar->hide();
    } else {
        m_tableLabel->setText("⚡ Translating...");
        m_miniProgressBar->show();
    }
}

void GlobalStatusWidget::updateProgress(int current, int total)
{
    if (total <= 0) return;
    int percent = (current * 100) / total;
    m_miniProgressBar->setValue(percent);
    m_miniProgressBar->show();

    if (current >= total) {
        setTableTranslationActive(false);
        m_tableLabel->setText("Text: Translation Finished ✅");
    } else {
        updateStatusIndicator(m_tableIndicator, true);
        m_tableLabel->setText(QString("⚡ Translating: %1/%2 (%3%)").arg(current).arg(total).arg(percent));
    }
}

void GlobalStatusWidget::resetProgress()
{
    setTableTranslationActive(false);
}

void GlobalStatusWidget::updateStatusIndicator(QLabel *indicator, bool active)
{
    QString color = active ? "#00ffaa" : "#9E9E9E";
    indicator->setStyleSheet(QString(
        "QLabel { "
        "background-color: %1; "
        "border-radius: 5px; "
        "}"
    ).arg(color));
}

void GlobalStatusWidget::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    emit tableStatusClicked();
    QWidget::mousePressEvent(event);
}
