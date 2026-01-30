#ifndef NST_UI_IMAGEQUEUE_H
#define NST_UI_IMAGEQUEUE_H

#include <QListWidget>

namespace nst::ui {

/**
 * @brief Image queue component showing thumbnails of queued images.
 */
class ImageQueue : public QListWidget
{
    Q_OBJECT

public:
    explicit ImageQueue(QWidget *parent = nullptr);

    void addImage(const QString &path);
    void setImageStatus(int index, int status);
    void removeSelected();
    void clearAll();

signals:
    void imageSelected(int index);
    void imageRemoved(int index);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUI();
    void updateItemStatus(QListWidgetItem *item, int status);
};

} // namespace nst::ui

#endif // NST_UI_IMAGEQUEUE_H
