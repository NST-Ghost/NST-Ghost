#include "AppNavigation.h"
#include "../styles/Theme.h"

namespace nst::ui {

AppNavigation::AppNavigation(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void AppNavigation::setupUI()
{
    setFixedWidth(220);
    setObjectName("AppNavigation");
    
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(12, 16, 12, 16);
    m_layout->setSpacing(8);
    
    // Logo/Title
    QLabel *logo = new QLabel("NST", this);
    logo->setStyleSheet(QString(
        "font-size: 24px; font-weight: bold; color: %1; padding: 16px 0;"
    ).arg(Theme::Colors::primary().name()));
    logo->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(logo);
    
    m_layout->addSpacing(16);
    
    // Navigation buttons
    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true);
    
    m_btnFileTranslation = createNavButton("File Translation", "file");
    m_btnSettings = createNavButton("Settings", "settings");
    
    m_buttonGroup->addButton(m_btnFileTranslation, 0);
    m_buttonGroup->addButton(m_btnSettings, 1);
    
    m_layout->addWidget(m_btnFileTranslation);
    
    m_layout->addStretch();
    
    m_layout->addWidget(m_btnSettings);
    
    // Connect signals
    connect(m_buttonGroup, QOverload<int>::of(&QButtonGroup::idClicked),
            this, &AppNavigation::navigationRequested);
    
    // Default selection
    m_btnFileTranslation->setChecked(true);
    
    // Stylesheet
    setStyleSheet(QString(R"(
        #AppNavigation {
            background-color: %1;
            border-right: 1px solid %2;
        }
        
        QPushButton {
            text-align: left;
            padding: 12px 16px;
            border: none;
            border-radius: 8px;
            background: transparent;
            color: %3;
            font-size: 13px;
        }
        
        QPushButton:hover {
            background-color: %4;
        }
        
        QPushButton:checked {
            background-color: %5;
            color: white;
            font-weight: 500;
        }
    )").arg(
        Theme::Colors::surface().name(),
        Theme::Colors::border().name(),
        Theme::Colors::textSecondary().name(),
        Theme::Colors::surfaceLight().name(),
        Theme::Colors::primary().name()
    ));
}

QPushButton* AppNavigation::createNavButton(const QString &text, const QString &/*icon*/)
{
    QPushButton *btn = new QPushButton(text, this);
    btn->setCheckable(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumHeight(44);
    return btn;
}

void AppNavigation::setActiveIndex(int index)
{
    if (QAbstractButton *btn = m_buttonGroup->button(index)) {
        btn->setChecked(true);
    }
}

} // namespace nst::ui
