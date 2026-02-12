#include "customtitlebar.h"
#include <QStyle>
#include <QMouseEvent>
#include <QApplication>
#include <QButtonGroup>

CustomTitleBar::CustomTitleBar(QWidget *parent)
    : QWidget(parent), m_isDrag(false)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(10, 5, 10, 5);
    m_layout->setSpacing(8);

    // Icon and title labels (hidden, kept for API compatibility)
    m_iconLabel = new QLabel(this);
    m_iconLabel->setVisible(false);
    m_titleLabel = new QLabel(this);
    m_titleLabel->setVisible(false);
    
    // Navigation Buttons (styled as tabs)
    m_fileTransButton = new QPushButton(this);
    m_fileTransButton->setObjectName("navButton");
    m_fileTransButton->setText("File Translate");
    m_fileTransButton->setCheckable(true);
    m_fileTransButton->setChecked(true);
    m_fileTransButton->setCursor(Qt::PointingHandCursor);

    m_realTimeButton = new QPushButton(this);
    m_realTimeButton->setObjectName("navButton");
    m_realTimeButton->setText("Real-time");
    m_realTimeButton->setCheckable(true);
    m_realTimeButton->setCursor(Qt::PointingHandCursor);

    m_imageTransButton = new QPushButton(this);
    m_imageTransButton->setObjectName("navButton");
    m_imageTransButton->setText("Image Trans");
    m_imageTransButton->setCheckable(true);
    m_imageTransButton->setCursor(Qt::PointingHandCursor);

    
    // Exclusive checking (tab behavior)
    QButtonGroup *navGroup = new QButtonGroup(this);
    navGroup->addButton(m_fileTransButton);
    navGroup->addButton(m_realTimeButton);
    navGroup->addButton(m_imageTransButton);
    navGroup->setExclusive(true);
    
    // Window controls (hidden - OS handles these now)
    m_minimizeButton = new QPushButton(this);
    m_minimizeButton->setVisible(false);
    m_maximizeButton = new QPushButton(this);
    m_maximizeButton->setVisible(false);
    m_closeButton = new QPushButton(this);
    m_closeButton->setVisible(false);

    // Add only navigation buttons to layout
    m_layout->addWidget(m_fileTransButton);
    m_layout->addWidget(m_realTimeButton);
    m_layout->addWidget(m_imageTransButton);
    m_layout->addStretch();

    // Height for navigation bar
    setFixedHeight(36);

    // Connect navigation signals
    connect(m_fileTransButton, &QPushButton::clicked, this, &CustomTitleBar::translateModeClicked);
    connect(m_realTimeButton, &QPushButton::clicked, this, &CustomTitleBar::realTimeModeClicked);
    connect(m_imageTransButton, &QPushButton::clicked, this, &CustomTitleBar::imageTranslationClicked);
}

void CustomTitleBar::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void CustomTitleBar::setIcon(const QIcon &icon)
{
    m_iconLabel->setPixmap(icon.pixmap(20, 20));
}


void CustomTitleBar::setRealTimeVisible(bool visible)
{
    m_realTimeButton->setVisible(visible);
}

void CustomTitleBar::setImageTransVisible(bool visible)
{
    m_imageTransButton->setVisible(visible);
}

// Mouse events - no longer needed, OS handles window movement
void CustomTitleBar::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}

void CustomTitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget::mouseDoubleClickEvent(event);
}
