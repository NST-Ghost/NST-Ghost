#include "virtualtranslationmodel.h"

VirtualTranslationModel::VirtualTranslationModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int VirtualTranslationModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

int VirtualTranslationModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return ColumnCount;
}

QVariant VirtualTranslationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const TranslationRowItem &item = m_items.at(index.row());

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
            case ColumnContext:     return item.context;
            case ColumnSourceText:  return item.sourceText;
            case ColumnTranslation: return item.translation;
            default: break;
        }
    } else if (role == Qt::UserRole + 2) {
        // Warning data role
        return item.warning;
    } else if (role == Qt::UserRole + 3) {
        // Ignored flag role
        return item.ignored;
    } else if (role == Qt::ForegroundRole) {
        if (item.ignored) {
            return QColor(128, 128, 128); // Grayed out for ignored
        }
        if (!item.warning.isEmpty()) {
            return QColor(255, 170, 0); // Orange highlight for warnings
        }
    }

    return QVariant();
}

bool VirtualTranslationModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return false;

    TranslationRowItem &item = m_items[index.row()];
    bool changed = false;

    if (role == Qt::EditRole || role == Qt::DisplayRole) {
        if (index.column() == ColumnTranslation) {
            item.translation = value.toString();
            changed = true;
        } else if (index.column() == ColumnContext) {
            item.context = value.toString();
            changed = true;
        } else if (index.column() == ColumnSourceText) {
            item.sourceText = value.toString();
            changed = true;
        }
    } else if (role == Qt::UserRole + 2) {
        item.warning = value.toString();
        changed = true;
    } else if (role == Qt::UserRole + 3) {
        item.ignored = value.toBool();
        changed = true;
    }

    if (changed) {
        emit dataChanged(index, index, {role, Qt::DisplayRole});
        return true;
    }

    return false;
}

QVariant VirtualTranslationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case ColumnContext:     return QString("Context");
            case ColumnSourceText:  return QString("Source Text");
            case ColumnTranslation: return QString("Translation");
            default: break;
        }
    } else if (orientation == Qt::Vertical && role == Qt::DisplayRole) {
        return section + 1; // 1-indexed row numbers
    }
    return QVariant();
}

Qt::ItemFlags VirtualTranslationModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == ColumnTranslation) {
        flags |= Qt::ItemIsEditable; // Allow editing translation column
    }
    return flags;
}

void VirtualTranslationModel::setRows(const QVector<TranslationRowItem> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

void VirtualTranslationModel::appendRow(const TranslationRowItem &item)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(item);
    endInsertRows();
}

void VirtualTranslationModel::updateTranslation(int row, const QString &translatedText)
{
    if (row < 0 || row >= m_items.size()) return;
    m_items[row].translation = translatedText;
    QModelIndex idx = index(row, ColumnTranslation);
    emit dataChanged(idx, idx, {Qt::DisplayRole, Qt::EditRole});
}

void VirtualTranslationModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

TranslationRowItem VirtualTranslationModel::itemAt(int row) const
{
    if (row >= 0 && row < m_items.size()) {
        return m_items.at(row);
    }
    return TranslationRowItem();
}
