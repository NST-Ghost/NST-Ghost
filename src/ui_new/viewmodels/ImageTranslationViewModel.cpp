#include "ImageTranslationViewModel.h"
#include "translationservicemanager.h"
#include "imageprocessorworker.h"

#include <QThread>
#include <QFileDialog>
#include <QFileInfo>

namespace nst::ui {

ImageTranslationViewModel::ImageTranslationViewModel(TranslationServiceManager *translationService,
                                                      QObject *parent)
    : BaseViewModel(parent)
    , m_translationService(translationService)
{
    initializeWorker();
}

ImageTranslationViewModel::~ImageTranslationViewModel()
{
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void ImageTranslationViewModel::initializeWorker()
{
    m_workerThread = new QThread(this);
    m_worker = new ImageProcessorWorker();
    m_worker->moveToThread(m_workerThread);
    
    connect(m_workerThread, &QThread::started, m_worker, &ImageProcessorWorker::initialize);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    
    connect(m_worker, &ImageProcessorWorker::initialized, this, &ImageTranslationViewModel::onWorkerInitialized);
    connect(m_worker, &ImageProcessorWorker::progress, this, &ImageTranslationViewModel::onWorkerProgress);
    connect(m_worker, &ImageProcessorWorker::processingFinished, this, &ImageTranslationViewModel::onWorkerFinished);
    connect(m_worker, &ImageProcessorWorker::errorOccurred, this, &ImageTranslationViewModel::onWorkerError);
    
    m_workerThread->start();
}

void ImageTranslationViewModel::addImages()
{
    QStringList paths = QFileDialog::getOpenFileNames(nullptr, "Add Images", "", 
                                                       "Images (*.png *.jpg *.jpeg *.bmp)");
    
    for (const QString &path : paths) {
        if (!m_imagePaths.contains(path)) {
            m_imagePaths.append(path);
            m_imageStatuses.append(Pending);
            m_imageDetections.append(QJsonArray());
            m_imageTranslations.append(QStringList());
            m_inpaintedPaths.append(QString());
        }
    }
    
    emit imageQueueChanged();
    
    if (m_currentIndex < 0 && !m_imagePaths.isEmpty()) {
        selectImage(0);
    }
}

void ImageTranslationViewModel::selectImage(int index)
{
    if (index < 0 || index >= m_imagePaths.size()) return;
    
    m_currentIndex = index;
    m_currentImagePath = m_imagePaths[index];
    m_currentPixmap = QPixmap(m_currentImagePath);
    m_detections = m_imageDetections[index];
    m_translations = m_imageTranslations[index];
    m_currentInpaintedPath = m_inpaintedPaths[index];
    
    if (!m_currentInpaintedPath.isEmpty()) {
        m_inpaintedPixmap = QPixmap(m_currentInpaintedPath);
    } else {
        m_inpaintedPixmap = QPixmap();
    }
    
    updateCurrentDisplay();
}

void ImageTranslationViewModel::removeImage(int index)
{
    if (index < 0 || index >= m_imagePaths.size()) return;
    
    m_imagePaths.removeAt(index);
    m_imageStatuses.removeAt(index);
    m_imageDetections.removeAt(index);
    m_imageTranslations.removeAt(index);
    m_inpaintedPaths.removeAt(index);
    
    emit imageQueueChanged();
    
    if (m_imagePaths.isEmpty()) {
        m_currentIndex = -1;
        m_currentImagePath.clear();
        m_currentPixmap = QPixmap();
        emit currentImageChanged(m_currentPixmap);
    } else if (index <= m_currentIndex) {
        selectImage(qMax(0, m_currentIndex - 1));
    }
}

void ImageTranslationViewModel::clearAll()
{
    m_imagePaths.clear();
    m_imageStatuses.clear();
    m_imageDetections.clear();
    m_imageTranslations.clear();
    m_inpaintedPaths.clear();
    
    m_currentIndex = -1;
    m_currentImagePath.clear();
    m_currentPixmap = QPixmap();
    m_inpaintedPixmap = QPixmap();
    m_detections = QJsonArray();
    m_translations.clear();
    
    emit imageQueueChanged();
    emit currentImageChanged(m_currentPixmap);
}

void ImageTranslationViewModel::translateCurrent()
{
    if (m_currentIndex < 0) return;
    
    setBusy(true);
    m_imageStatuses[m_currentIndex] = Processing;
    emit imageStatusChanged(m_currentIndex, Processing);
    
    processImage(m_currentIndex);
}

void ImageTranslationViewModel::translateAll()
{
    if (m_imagePaths.isEmpty()) return;
    
    m_isBatchProcessing = true;
    m_cancelRequested = false;
    setBusy(true);
    
    // Start with first pending
    for (int i = 0; i < m_imagePaths.size(); ++i) {
        if (m_imageStatuses[i] == Pending) {
            selectImage(i);
            m_imageStatuses[i] = Processing;
            emit imageStatusChanged(i, Processing);
            processImage(i);
            return;
        }
    }
    
    setBusy(false);
    m_isBatchProcessing = false;
}

void ImageTranslationViewModel::stopTranslation()
{
    m_cancelRequested = true;
    setStatusMessage("Stopping...");
}

void ImageTranslationViewModel::processImage(int index)
{
    if (index < 0 || index >= m_imagePaths.size()) return;
    
    QString imagePath = m_imagePaths[index];
    
    // Request processing from worker
    // Worker signal: processImage(path, sourceLanguage, useGcv, gcvKey)
    QMetaObject::invokeMethod(m_worker, "processImage",
                               Q_ARG(QString, imagePath),
                               Q_ARG(QString, m_sourceLanguage),
                               Q_ARG(bool, false),
                               Q_ARG(QString, QString()));
}

void ImageTranslationViewModel::setViewMode(int mode)
{
    m_viewMode = static_cast<ViewMode>(mode);
    updateCurrentDisplay();
}

void ImageTranslationViewModel::peekOriginal()
{
    m_savedViewMode = m_viewMode;
    m_viewMode = Original;
    updateCurrentDisplay();
}

void ImageTranslationViewModel::restoreView()
{
    m_viewMode = m_savedViewMode;
    updateCurrentDisplay();
}

void ImageTranslationViewModel::saveCurrentImage()
{
    if (m_currentImagePath.isEmpty()) return;
    
    QString savePath = QFileDialog::getSaveFileName(nullptr, "Save Translation", "", 
                                                     "Images (*.png *.jpg)");
    if (!savePath.isEmpty()) {
        QPixmap toSave;
        
        if (m_viewMode == Translated && !m_inpaintedPixmap.isNull()) {
            // TODO: Render translated text onto pixmap
            toSave = m_inpaintedPixmap;
        } else if (m_viewMode == Clean && !m_inpaintedPixmap.isNull()) {
            toSave = m_inpaintedPixmap;
        } else {
            toSave = m_currentPixmap;
        }
        
        toSave.save(savePath);
        setStatusMessage("Saved: " + QFileInfo(savePath).fileName());
    }
}

void ImageTranslationViewModel::updateCurrentDisplay()
{
    QPixmap display;
    
    switch (m_viewMode) {
        case Original:
            display = m_currentPixmap;
            break;
        case Clean:
        case Translated:
            display = m_inpaintedPixmap.isNull() ? m_currentPixmap : m_inpaintedPixmap;
            break;
    }
    
    emit currentImageChanged(display);
    emit detectionsChanged(m_detections, m_translations);
}

void ImageTranslationViewModel::onWorkerInitialized(bool success, const QString &status)
{
    if (success) {
        setStatusMessage("Ready: " + status);
    } else {
        reportError("Worker initialization failed: " + status);
    }
}

void ImageTranslationViewModel::onWorkerProgress(const QString &message)
{
    setStatusMessage(message);
}

void ImageTranslationViewModel::onWorkerFinished(const QString &imagePath, 
                                                   const QJsonArray &detections,
                                                   const QString &inpaintedPath)
{
    int idx = m_imagePaths.indexOf(imagePath);
    if (idx < 0) return;
    
    m_imageDetections[idx] = detections;
    m_inpaintedPaths[idx] = inpaintedPath;
    
    if (idx == m_currentIndex) {
        m_detections = detections;
        m_currentInpaintedPath = inpaintedPath;
        m_inpaintedPixmap = QPixmap(inpaintedPath);
    }
    
    // TODO: Request translations from TranslationServiceManager
    // For now, mark as completed
    m_imageStatuses[idx] = Completed;
    emit imageStatusChanged(idx, Completed);
    
    updateCurrentDisplay();
    
    // Continue batch if needed
    if (m_isBatchProcessing && !m_cancelRequested) {
        for (int i = 0; i < m_imagePaths.size(); ++i) {
            if (m_imageStatuses[i] == Pending) {
                selectImage(i);
                m_imageStatuses[i] = Processing;
                emit imageStatusChanged(i, Processing);
                processImage(i);
                return;
            }
        }
        
        // All done
        m_isBatchProcessing = false;
        setBusy(false);
        setStatusMessage("Batch complete.");
    } else {
        setBusy(false);
        setStatusMessage("Finished.");
    }
}

void ImageTranslationViewModel::onWorkerError(const QString &message)
{
    if (m_currentIndex >= 0) {
        m_imageStatuses[m_currentIndex] = Error;
        emit imageStatusChanged(m_currentIndex, Error);
    }
    
    reportError(message);
    setBusy(false);
}

void ImageTranslationViewModel::onTranslationFinished(int index, const QString &translation)
{
    if (index >= 0 && index < m_translations.size()) {
        m_translations[index] = translation;
        
        if (m_currentIndex >= 0 && m_currentIndex < m_imageTranslations.size()) {
            m_imageTranslations[m_currentIndex] = m_translations;
        }
        
        updateCurrentDisplay();
    }
}

} // namespace nst::ui
