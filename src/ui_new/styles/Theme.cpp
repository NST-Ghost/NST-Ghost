#include "Theme.h"
#include <QFile>
#include <QFontDatabase>

namespace nst::ui {

QFont Theme::Fonts::body()
{
    QFont font("Segoe UI", 10);
    font.setWeight(QFont::Normal);
    return font;
}

QFont Theme::Fonts::title()
{
    QFont font("Segoe UI", 14);
    font.setWeight(QFont::DemiBold);
    return font;
}

QFont Theme::Fonts::heading()
{
    QFont font("Segoe UI", 12);
    font.setWeight(QFont::Medium);
    return font;
}

QFont Theme::Fonts::mono()
{
    QFont font("Consolas", 10);
    font.setWeight(QFont::Normal);
    return font;
}

QString Theme::loadStylesheet()
{
    QFile file(":/styles/main.qss");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    
    // Fallback inline stylesheet if resource not found
    return QString(R"(
        /* Base Application Styles */
        QWidget {
            background-color: #1a1a2e;
            color: #eaeaea;
            font-family: "Segoe UI", sans-serif;
            font-size: 10pt;
        }
        
        /* Buttons */
        QPushButton {
            background-color: #e94560;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: 500;
        }
        
        QPushButton:hover {
            background-color: #ff6b8a;
        }
        
        QPushButton:pressed {
            background-color: #c8374f;
        }
        
        QPushButton:disabled {
            background-color: #444;
            color: #888;
        }
        
        /* Secondary Button */
        QPushButton[class="secondary"] {
            background-color: #0f3460;
            border: 1px solid #2a2a4a;
        }
        
        QPushButton[class="secondary"]:hover {
            background-color: #1a4a7a;
        }
        
        /* Line Edits */
        QLineEdit, QTextEdit, QPlainTextEdit {
            background-color: #16213e;
            border: 1px solid #2a2a4a;
            border-radius: 6px;
            padding: 8px;
            color: #eaeaea;
        }
        
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
            border-color: #e94560;
        }
        
        /* Table Views */
        QTableView, QTreeView, QListView {
            background-color: #16213e;
            alternate-background-color: #1a2640;
            border: 1px solid #2a2a4a;
            border-radius: 8px;
            gridline-color: #2a2a4a;
        }
        
        QTableView::item:selected, QTreeView::item:selected, QListView::item:selected {
            background-color: #e94560;
            color: white;
        }
        
        QHeaderView::section {
            background-color: #0f3460;
            color: #eaeaea;
            padding: 8px;
            border: none;
            border-bottom: 1px solid #2a2a4a;
        }
        
        /* Scroll Bars */
        QScrollBar:vertical {
            background: #16213e;
            width: 10px;
            border-radius: 5px;
        }
        
        QScrollBar::handle:vertical {
            background: #444;
            border-radius: 5px;
            min-height: 30px;
        }
        
        QScrollBar::handle:vertical:hover {
            background: #e94560;
        }
        
        /* Tab Widget */
        QTabWidget::pane {
            border: 1px solid #2a2a4a;
            border-radius: 8px;
        }
        
        QTabBar::tab {
            background: #0f3460;
            color: #a0a0a0;
            padding: 10px 20px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
        }
        
        QTabBar::tab:selected {
            background: #e94560;
            color: white;
        }
        
        /* Progress Bar */
        QProgressBar {
            background-color: #0f3460;
            border: none;
            border-radius: 4px;
            height: 8px;
            text-align: center;
        }
        
        QProgressBar::chunk {
            background-color: #e94560;
            border-radius: 4px;
        }
        
        /* Combo Box */
        QComboBox {
            background-color: #16213e;
            border: 1px solid #2a2a4a;
            border-radius: 6px;
            padding: 8px;
            color: #eaeaea;
        }
        
        QComboBox:hover {
            border-color: #e94560;
        }
        
        QComboBox::drop-down {
            border: none;
            width: 30px;
        }
        
        QComboBox QAbstractItemView {
            background-color: #16213e;
            border: 1px solid #2a2a4a;
            selection-background-color: #e94560;
        }
        
        /* Group Box */
        QGroupBox {
            font-weight: 600;
            border: 1px solid #2a2a4a;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 12px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 4px;
            color: #e94560;
        }
        
        /* Check Box */
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 4px;
            border: 2px solid #2a2a4a;
            background: #16213e;
        }
        
        QCheckBox::indicator:checked {
            background: #e94560;
            border-color: #e94560;
        }
        
        /* Splitter */
        QSplitter::handle {
            background-color: #2a2a4a;
        }
        
        QSplitter::handle:hover {
            background-color: #e94560;
        }
    )");
}

} // namespace nst::ui
