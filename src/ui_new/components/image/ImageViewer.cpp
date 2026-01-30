#include "ImageViewer.h"
#include "../../styles/Theme.h"

#include <QWheelEvent>
#include <QGraphicsPixmapItem>
#include <QJsonObject>

namespace nst::ui {

ImageViewer::ImageViewer(QWidget *parent)
    : QGraphicsView(parent)
{
    setupUI();
}

ImageViewer::~ImageViewer() = default;

void ImageViewer::setupUI()
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    // View configuration
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Styling
    setStyleSheet(QString(R"(
        QGraphicsView {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(Theme::Colors::surface().name())
       .arg(Theme::Colors::border().name()));
}

void ImageViewer::setImage(const QPixmap &pixmap)
{
    m_currentPixmap = pixmap;
    m_scene->clear();
    m_imageItem = nullptr;
    
    if (!pixmap.isNull()) {
        m_imageItem = m_scene->addPixmap(pixmap);
        m_scene->setSceneRect(pixmap.rect());
        
        if (m_fitOnResize) {
            fitToView();
        }
    }
    
    renderOverlays();
}

void ImageViewer::setDetections(const QJsonArray &detections, const QStringList &translations)
{
    m_detections = detections;
    m_translations = translations;
    renderOverlays();
}

void ImageViewer::clear()
{
    m_scene->clear();
    m_imageItem = nullptr;
    m_currentPixmap = QPixmap();
    m_detections = QJsonArray();
    m_translations.clear();
}

void ImageViewer::zoomIn()
{
    m_fitOnResize = false;
    m_zoomFactor *= 1.25;
    setTransform(QTransform::fromScale(m_zoomFactor, m_zoomFactor));
}

void ImageViewer::zoomOut()
{
    m_fitOnResize = false;
    m_zoomFactor *= 0.8;
    setTransform(QTransform::fromScale(m_zoomFactor, m_zoomFactor));
}

void ImageViewer::resetZoom()
{
    m_fitOnResize = false;
    m_zoomFactor = 1.0;
    setTransform(QTransform());
}

void ImageViewer::fitToView()
{
    m_fitOnResize = true;
    if (m_imageItem) {
        fitInView(m_imageItem, Qt::KeepAspectRatio);
        m_zoomFactor = transform().m11();
    }
}

void ImageViewer::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void ImageViewer::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    
    if (m_fitOnResize && m_imageItem) {
        fitToView();
    }
}

void ImageViewer::renderOverlays()
{
    // Remove existing overlay items (keep only the image)
    for (QGraphicsItem *item : m_scene->items()) {
        if (item != m_imageItem) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    
    // Draw detection boxes with translations
    for (int i = 0; i < m_detections.size(); ++i) {
        QJsonObject obj = m_detections[i].toObject();
        QJsonArray bbox = obj["bbox"].toArray();
        
        if (bbox.size() < 4) continue;
        
        // Get bounding box coordinates
        QJsonArray p1 = bbox[0].toArray();
        QJsonArray p3 = bbox[2].toArray();
        
        int x = p1[0].toInt();
        int y = p1[1].toInt();
        int w = p3[0].toInt() - x;
        int h = p3[1].toInt() - y;
        
        QRectF rect(x, y, w, h);
        
        // Draw box
        QPen pen(Theme::Colors::primary());
        pen.setWidth(2);
        m_scene->addRect(rect, pen);
        
        // Draw translation text if available
        if (i < m_translations.size() && !m_translations[i].isEmpty()) {
            QGraphicsTextItem *text = m_scene->addText(m_translations[i]);
            text->setDefaultTextColor(Theme::Colors::textPrimary());
            text->setPos(x, y - 20);
        }
    }
}

} // namespace nst::ui
