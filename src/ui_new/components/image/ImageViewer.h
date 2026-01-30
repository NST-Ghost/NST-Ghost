#ifndef NST_UI_IMAGEVIEWER_H
#define NST_UI_IMAGEVIEWER_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QJsonArray>

namespace nst::ui {

/**
 * @brief Image viewer component with zoom/pan and overlay rendering.
 */
class ImageViewer : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = nullptr);
    ~ImageViewer() override;

    void setImage(const QPixmap &pixmap);
    void setDetections(const QJsonArray &detections, const QStringList &translations);
    void clear();

    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitToView();

signals:
    void detectionClicked(int index);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUI();
    void renderOverlays();

private:
    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_imageItem = nullptr;
    
    QPixmap m_currentPixmap;
    QJsonArray m_detections;
    QStringList m_translations;
    
    double m_zoomFactor = 1.0;
    bool m_fitOnResize = true;
};

} // namespace nst::ui

#endif // NST_UI_IMAGEVIEWER_H
