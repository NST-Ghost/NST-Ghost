#include "TranslationTable.h"
#include "../../styles/Theme.h"

#include <QHeaderView>
#include <QMenu>
#include <QAction>

namespace nst::ui {

TranslationTable::TranslationTable(QWidget *parent)
    : QTableView(parent)
{
    setupUI();
    setupContextMenu();
}

void TranslationTable::setupUI()
{
    // Table configuration
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    setWordWrap(true);
    
    // Header configuration
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setMinimumSectionSize(150);
    
    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(50);
    
    // Styling
    setStyleSheet(QString(R"(
        QTableView {
            background-color: %1;
            alternate-background-color: %2;
            border: 1px solid %3;
            border-radius: 8px;
            gridline-color: %4;
        }
        
        QTableView::item {
            padding: 8px;
            border-bottom: 1px solid %4;
        }
        
        QTableView::item:selected {
            background-color: %5;
            color: white;
        }
        
        QTableView::item:hover:!selected {
            background-color: %6;
        }
        
        QHeaderView::section {
            background-color: %7;
            color: %8;
            padding: 10px;
            border: none;
            border-bottom: 2px solid %5;
            font-weight: 600;
        }
    )")
    .arg(Theme::Colors::surface().name())
    .arg(Theme::Colors::surface().lighter(110).name())
    .arg(Theme::Colors::border().name())
    .arg(Theme::Colors::border().name())
    .arg(Theme::Colors::primary().name())
    .arg(Theme::Colors::surfaceLight().name())
    .arg(Theme::Colors::secondary().name())
    .arg(Theme::Colors::textPrimary().name()));
}

void TranslationTable::setupContextMenu()
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    
    connect(this, &QTableView::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu(this);
        
        QAction *translateAction = menu.addAction("🌐 Translate Selected");
        QAction *copyAction = menu.addAction("📋 Copy Source");
        menu.addSeparator();
        QAction *ignoreAction = menu.addAction("🚫 Mark as Ignored");
        
        connect(translateAction, &QAction::triggered, this, [this]() {
            // TODO: Emit translate signal
        });
        
        connect(copyAction, &QAction::triggered, this, [this]() {
            // TODO: Copy to clipboard
        });
        
        connect(ignoreAction, &QAction::triggered, this, [this]() {
            // TODO: Mark as ignored
        });
        
        menu.exec(mapToGlobal(pos));
    });
}

void TranslationTable::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);
    
    if (model) {
        connect(model, &QAbstractItemModel::dataChanged,
                this, &TranslationTable::onDataChanged);
        
        // Set column widths
        if (model->columnCount() >= 2) {
            setColumnWidth(0, 400);  // Source text
            setColumnWidth(1, 400);  // Translation
        }
    }
}

void TranslationTable::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTableView::currentChanged(current, previous);
    emit selectionChanged(current);
}

void TranslationTable::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    // Track edits to translation column (column 1)
    if (topLeft.column() <= 1 && bottomRight.column() >= 1) {
        for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
            QModelIndex sourceIdx = model()->index(row, 0);
            QModelIndex transIdx = model()->index(row, 1);
            emit translationEdited(sourceIdx.data().toString(), transIdx.data().toString());
        }
    }
}

} // namespace nst::ui
