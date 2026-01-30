#ifndef NST_UI_TRANSLATIONTABLE_H
#define NST_UI_TRANSLATIONTABLE_H

#include <QTableView>
#include <QStandardItemModel>

namespace nst::ui {

/**
 * @brief Translation table component showing source/translation pairs.
 * 
 * Displays text entries with editable translation column.
 */
class TranslationTable : public QTableView
{
    Q_OBJECT

public:
    explicit TranslationTable(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

signals:
    void translationEdited(const QString &source, const QString &translation);
    void selectionChanged(const QModelIndex &current);

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

private:
    void setupUI();
    void setupContextMenu();

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
};

} // namespace nst::ui

#endif // NST_UI_TRANSLATIONTABLE_H
