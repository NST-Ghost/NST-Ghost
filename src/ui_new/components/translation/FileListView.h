#ifndef NST_UI_FILELISTVIEW_H
#define NST_UI_FILELISTVIEW_H

#include <QTreeView>
#include <QStandardItemModel>

namespace nst::ui {

/**
 * @brief File list component for displaying project files.
 * 
 * Shows game files in a tree structure with status indicators.
 */
class FileListView : public QTreeView
{
    Q_OBJECT

public:
    explicit FileListView(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

signals:
    void fileSelected(const QModelIndex &index);

protected:
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected) override;

private:
    void setupUI();
};

} // namespace nst::ui

#endif // NST_UI_FILELISTVIEW_H
