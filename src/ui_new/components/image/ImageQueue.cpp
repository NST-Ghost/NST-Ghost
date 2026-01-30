#include "ImageQueue.h"
#include "../../styles/Theme.h"

#include <QFileInfo>
#include <QKeyEvent>

namespace nst::ui {

ImageQueue::ImageQueue(QWidget *parent)
    : QListWidget(parent)
{
    setupUI();
}

void ImageQueue::setupUI()
{
    setViewMode(QListView::ListMode);
    setIconSize(QSize(60, 60));
    setSpacing(4);
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    
    setStyleSheet(QString(R"(
        QListWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
        
        QListWidget::item {
            padding: 8px;
            border-radius: 4px;
            border-bottom: 1px solid %3;
        }
        
        QListWidget::item:selected {
            background-color: %4;
            color: white;
        }
        
        QListWidget::item:hover:!selected {
            background-color: %5;
        }
    )").arg(Theme::Colors::surface().name())
       .arg(Theme::Colors::border().name())
       .arg(Theme::Colors::border().darker(110).name())
       .arg(Theme::Colors::primary().name())
       .arg(Theme::Colors::surfaceLight().name()));
    
    connect(this, &QListWidget::currentRowChanged, this, &ImageQueue::imageSelected);
}

void ImageQueue::addImage(const QString &path)
{
    QFileInfo fi(path);
    QListWidgetItem *item = new QListWidgetItem(fi.fileName());
    
    QPixmap thumb(path);
    if (!thumb.isNull()) {
        item->setIcon(QIcon(thumb.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }
    
    item->setToolTip(path);
    item->setData(Qt::UserRole, path);
    item->setData(Qt::UserRole + 1, 0);  // Status: Pending
    
    addItem(item);
}

void ImageQueue::setImageStatus(int index, int status)
{
    if (index >= 0 && index < count()) {
        QListWidgetItem *item = this->item(index);
        updateItemStatus(item, status);
    }
}

void ImageQueue::updateItemStatus(QListWidgetItem *item, int status)
{
    QString statusEmoji;
    switch (status) {
        case 0: statusEmoji = "🕐"; break;  // Pending
        case 1: statusEmoji = "⏳"; break;  // Processing
        case 2: statusEmoji = "✅"; break;  // Completed
        case 3: statusEmoji = "❌"; break;  // Error
    }
    
    QString path = item->data(Qt::UserRole).toString();
    QFileInfo fi(path);
    item->setText(statusEmoji + " " + fi.fileName());
    item->setData(Qt::UserRole + 1, status);
}

void ImageQueue::removeSelected()
{
    int row = currentRow();
    if (row >= 0) {
        delete takeItem(row);
        emit imageRemoved(row);
    }
}

void ImageQueue::clearAll()
{
    clear();
}

void ImageQueue::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        removeSelected();
        event->accept();
    } else {
        QListWidget::keyPressEvent(event);
    }
}

} // namespace nst::ui
