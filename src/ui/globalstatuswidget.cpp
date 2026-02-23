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

    layout->addWidget(m_tableIndicator);
    layout->addWidget(m_tableLabel);
    layout->addStretch(); // Add stretch to absorb remaining space

    // Initialize to inactive (gray)
    setTableTranslationActive(false);
}

void GlobalStatusWidget::setTableTranslationActive(bool active)
{
    updateStatusIndicator(m_tableIndicator, active);
    m_tableLabel->setText(active ? "Text: Translating..." : "Text: Ready");
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
    } else {
        // Broad click handling based on half screen if they just click the general area
        emit tableStatusClicked();
    }
    
    QWidget::mousePressEvent(event);
}
