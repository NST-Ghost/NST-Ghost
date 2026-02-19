#ifndef IMAGETRANSLATIONWIDGET_H
#define IMAGETRANSLATIONWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QScrollArea>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLineEdit>
#include <QJsonArray>
#include <QButtonGroup>
#include <QListWidget>
#include <QSplitter>
#include <QCheckBox>
#include <qtlingo/translationservice.h>

class TranslationServiceManager;
class ImageProcessorWorker;
class QThread;

namespace Ui {
class ImageTranslationWidget;
}

class ImageTranslationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ImageTranslationWidget(TranslationServiceManager *translationManager, QWidget *parent = nullptr);
    ~ImageTranslationWidget();
    
    void setSettings(const QString &apiKey, const QString &targetLanguage, bool googleApi,
                     const QString &llmProvider, const QString &llmApiKey, 
                     const QString &llmModel, const QString &llmBaseUrl,
                     int translationMode = 0, // Default to 0 (Quick)
                     const QString &sourceLanguage = "auto");

public slots:
    void onAddImages();      // Multi-file dialog (replaces onLoadImage)
    void onRemoveImage();    // Remove selected from queue
    void onClearAll();       // Clear entire queue
    void onSaveImage();
    void onTranslate();      // Translate current image only
    void onTranslateAll();   // Batch translate all pending
    void onStopTranslation(); // Cancel operation
    void onViewModeChanged(int id);
    void onPeekPressed();
    void onPeekReleased();

private slots:
    void onTranslationFinished(const qtlingo::TranslationResult &result);
    void onTranslationError(const QString &message);
    void onImageSelected(int row); // Switch displayed image

private:
    void setupUi();
    void displayImage(const QString &path);
    void updateViewMode(); // Refreshes view based on current mode
    bool processImage(int index); // Process single image (OCR + inpaint + translate)
    void updateListItemStatus(int index, int status); // Update icon in list
    
    // View Modes
    enum ViewMode {
        Original,
        Clean,
        Translated
    };
    
    // Image Item for queue
    struct ImageItem {
        QString path;
        enum Status { Pending = 0, Processing = 1, Completed = 2, Error = 3 };
        Status status = Pending;
        QJsonArray detections;
        QStringList translatedTexts;
        QString inpaintedPath;
    };
    
    // Image queue
    QList<ImageItem> m_imageQueue;
    int m_currentQueueIndex = -1;
    bool m_isBatchProcessing = false;
    bool m_cancelRequested = false;
    
    // View Controls logic
    QButtonGroup *m_viewGroup;
    QGraphicsScene *m_imageScene;

private:
    Ui::ImageTranslationWidget *ui;
    
    // Legacy single-image (now per-item in queue)
    QString m_currentImagePath;
    QJsonArray m_detections;
    QStringList m_translatedTexts;
    bool m_hasDetected = false;
    
    ViewMode m_currentViewMode = Original;
    QString m_inpaintedImagePath;

    
    // Translation state
    TranslationServiceManager *m_translationManager;
    int m_currentTranslationIndex = 0;
    
    // Background Processing
    QThread *m_workerThread;
    ImageProcessorWorker *m_worker;
    
signals:
    void processImageRequested(const QString &path, const QString &sourceLang, bool useGcv, const QString &gcvKeyPath);
    
private slots:
    void onDevModeToggled(bool checked);
    // Worker slots
    void onWorkerInitialized(bool success, const QString &status, bool useGpu, const QString &deviceName);
    void onWorkerProgress(const QString &message);
    void onWorkerProcessingFinished(const QString &imagePath, const QJsonArray &detections, const QString &inpaintedPath);
    void onWorkerError(const QString &message);

private:
    // Settings
    QString m_apiKey;
    QString m_targetLanguage;
    bool m_googleApi = false;
    QString m_llmProvider;
    QString m_llmApiKey;
    QString m_llmModel;
    QString m_llmBaseUrl;
    
    // New: Track active mode
    int m_translationMode = 0;
    QString m_sourceLanguage = "auto";

    // GCV Controls (Programmatic)
    QCheckBox *m_chkUseGcv = nullptr;
    QLineEdit *m_editGcvKeyPath = nullptr;
    QPushButton *m_btnBrowseGcvKey = nullptr;
};

#endif // IMAGETRANSLATIONWIDGET_H
