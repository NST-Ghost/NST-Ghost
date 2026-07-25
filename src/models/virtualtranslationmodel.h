#ifndef VIRTUALTRANSLATIONMODEL_H
#define VIRTUALTRANSLATIONMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QString>
#include <QVariant>
#include <QColor>

struct TranslationRowItem {
    QString context;
    QString sourceText;
    QString translation;
    QString warning;
    bool ignored = false;
};

class VirtualTranslationModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColumnContext = 0,
        ColumnSourceText = 1,
        ColumnTranslation = 2,
        ColumnCount = 3
    };

    explicit VirtualTranslationModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setRows(const QVector<TranslationRowItem> &items);
    void appendRow(const TranslationRowItem &item);
    void updateTranslation(int row, const QString &translatedText);
    void clear();

    const QVector<TranslationRowItem>& items() const { return m_items; }
    TranslationRowItem itemAt(int row) const;

private:
    QVector<TranslationRowItem> m_items;
};

#endif // VIRTUALTRANSLATIONMODEL_H
