#ifndef NST_UI_IMAGETRANSLATIONVIEWMODEL_H
#define NST_UI_IMAGETRANSLATIONVIEWMODEL_H

#include "../viewmodels/BaseViewModel.h"
#include <QJsonArray>
#include <QStringList>
#include <QPixmap>

class TranslationServiceManager;
class QThread;
class ImageProcessorWorker;

namespace nst::ui {

/**
 * @brief ViewModel for Image Translation page.
 * 
 * Manages image queue, OCR processing, and translation overlay.
 */
class ImageTranslationViewModel : public BaseViewModel
{
    Q_OBJECT

public:
    enum ViewMode { Original = 0, Clean = 1, Translated = 2 };
    Q_ENUM(ViewMode)

    enum ImageStatus { Pending, Processing, Completed, Error };
    Q_ENUM(ImageStatus)

    explicit ImageTranslationViewModel(TranslationServiceManager *translationService,
                                        QObject *parent = nullptr);
    ~ImageTranslationViewModel() override;

    // Current image properties
    QString currentImagePath() const { return m_currentImagePath; }
    QPixmap currentPixmap() const { return m_currentPixmap; }
    QJsonArray currentDetections() const { return m_detections; }
    QStringList currentTranslations() const { return m_translations; }
    ViewMode viewMode() const { return m_viewMode; }
    
    int imageCount() const { return m_imagePaths.size(); }
    int currentIndex() const { return m_currentIndex; }

    // Settings
    void setSourceLanguage(const QString &lang) { m_sourceLanguage = lang; }
    void setTargetLanguage(const QString &lang) { m_targetLanguage = lang; }

public slots:
    // Image queue management
    void addImages();
    void selectImage(int index);
    void removeImage(int index);
    void clearAll();

    // Translation
    void translateCurrent();
    void translateAll();
    void stopTranslation();

    // View
    void setViewMode(int mode);
    void peekOriginal();
    void restoreView();

    // Save
    void saveCurrentImage();

signals:
    void imageQueueChanged();
    void currentImageChanged(const QPixmap &pixmap);
    void detectionsChanged(const QJsonArray &detections, const QStringList &translations);
    void imageStatusChanged(int index, ImageStatus status);

private slots:
    void onWorkerInitialized(bool success, const QString &status);
    void onWorkerProgress(const QString &message);
    void onWorkerFinished(const QString &imagePath, const QJsonArray &detections, const QString &inpaintedPath);
    void onWorkerError(const QString &message);
    void onTranslationFinished(int index, const QString &translation);

private:
    void initializeWorker();
    void updateCurrentDisplay();
    void processImage(int index);

private:
    TranslationServiceManager *m_translationService = nullptr;
    
    // Worker thread for OCR/inpainting
    QThread *m_workerThread = nullptr;
    ImageProcessorWorker *m_worker = nullptr;

    // Image queue
    QStringList m_imagePaths;
    QList<ImageStatus> m_imageStatuses;
    QList<QJsonArray> m_imageDetections;
    QList<QStringList> m_imageTranslations;
    QList<QString> m_inpaintedPaths;
    
    int m_currentIndex = -1;
    QString m_currentImagePath;
    QPixmap m_currentPixmap;
    QPixmap m_inpaintedPixmap;
    QJsonArray m_detections;
    QStringList m_translations;
    QString m_currentInpaintedPath;
    
    ViewMode m_viewMode = Original;
    ViewMode m_savedViewMode = Original;  // For peek
    
    // Settings
    QString m_sourceLanguage = "ja";
    QString m_targetLanguage = "th";
    
    // State
    bool m_isBatchProcessing = false;
    bool m_cancelRequested = false;
};

} // namespace nst::ui

#endif // NST_UI_IMAGETRANSLATIONVIEWMODEL_H
