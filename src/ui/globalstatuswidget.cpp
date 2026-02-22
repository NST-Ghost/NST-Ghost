#include "globalstatuswidget.h"
#include <QMouseEvent>
#include <QPalette>

GlobalStatusWidget::GlobalStatusWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(10);

    // Table Translation Status
    m_tableIndicator = new QLabel(this);
    m_tableIndicator->setFixedSize(10, 10);
    m_tableLabel = new QLabel("Table: Inactive", this);

    // Image Translation Status
    m_imageIndicator = new QLabel(this);
    m_imageIndicator->setFixedSize(10, 10);
    m_imageLabel = new QLabel("Image: Inactive", this);

    layout->addWidget(m_tableIndicator);
    layout->addWidget(m_tableLabel);
    layout->addSpacing(20);
    layout->addWidget(m_imageIndicator);
    layout->addWidget(m_imageLabel);

    // Initialize to inactive (gray)
    setTableTranslationActive(false);
    setImageTranslationActive(false);
}

void GlobalStatusWidget::setTableTranslationActive(bool active)
{
    updateStatusIndicator(m_tableIndicator, active);
    m_tableLabel->setText(active ? "Text: Translating..." : "Text: Ready");
}

void GlobalStatusWidget::setImageTranslationActive(bool active)
{
    updateStatusIndicator(m_imageIndicator, active);
    m_imageLabel->setText(active ? "Image: Translating..." : "Image: Ready");
}

void GlobalStatusWidget::updateStatusIndicator(QLabel *indicator, bool active)
{
    // Draw a colored circle using style sheets
    QString color = active ? "#4CAF50" : "#9E9E9E"; // Green if active, Gray if inactive
    indicator->setStyleSheet(QString(
        "QLabel { "
        "background-color: %1; "
        "border-radius: 5px; "
        "}"
    ).arg(color));
}

void GlobalStatusWidget::mousePressEvent(QMouseEvent *event)
{
    // Determine which status was clicked roughly by X coordinate or using childAt
    QWidget *clickedWidget = childAt(event->pos());
    
    if (clickedWidget == m_tableIndicator || clickedWidget == m_tableLabel) {
        emit tableStatusClicked();
    } else if (clickedWidget == m_imageIndicator || clickedWidget == m_imageLabel) {
        emit imageStatusClicked();
    } else {
        // Broad click handling based on half screen if they just click the general area
        if (event->pos().x() < width() / 2) {
             emit tableStatusClicked();
        } else {
             emit imageStatusClicked();
        }
    }
    
    QWidget::mousePressEvent(event);
}
