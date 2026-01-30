#include "FileListView.h"
#include "../../styles/Theme.h"

namespace nst::ui {

FileListView::FileListView(QWidget *parent)
    : QTreeView(parent)
{
    setupUI();
}

void FileListView::setupUI()
{
    setHeaderHidden(true);
    setRootIsDecorated(true);
    setAnimated(true);
    setIndentation(16);
    setUniformRowHeights(true);
    setAlternatingRowColors(true);
    
    setStyleSheet(QString(R"(
        QTreeView {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
        
        QTreeView::item {
            padding: 8px 4px;
            border-bottom: 1px solid %3;
        }
        
        QTreeView::item:selected {
            background-color: %4;
            color: white;
        }
        
        QTreeView::item:hover:!selected {
            background-color: %5;
        }
        
        QTreeView::branch:has-children:!has-siblings:closed,
        QTreeView::branch:closed:has-children:has-siblings {
            image: url(:/icons/chevron-right.svg);
        }
        
        QTreeView::branch:open:has-children:!has-siblings,
        QTreeView::branch:open:has-children:has-siblings {
            image: url(:/icons/chevron-down.svg);
        }
    )")
    .arg(Theme::Colors::surface().name())
    .arg(Theme::Colors::border().name())
    .arg(Theme::Colors::border().darker(110).name())
    .arg(Theme::Colors::primary().name())
    .arg(Theme::Colors::surfaceLight().name()));
}

void FileListView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    
    // Configure columns if model has them
    if (model && model->columnCount() > 1) {
        for (int i = 1; i < model->columnCount(); ++i) {
            hideColumn(i);
        }
    }
}

void FileListView::selectionChanged(const QItemSelection &selected,
                                     const QItemSelection &deselected)
{
    QTreeView::selectionChanged(selected, deselected);
    
    if (!selected.isEmpty()) {
        QModelIndex index = selected.indexes().first();
        emit fileSelected(index);
    }
}

} // namespace nst::ui
