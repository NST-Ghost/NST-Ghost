#include "imagetranslationwidget.h"
#include "ui_imagetranslationwidget.h"
#include "translationservicemanager.h"
#include "imageprocessorworker.h"
#include <QThread>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QClipboard>
#include <QApplication>
#include <QPen>
#include <QPainter>
#include <QFontMetrics>
#include <QDateTime>
#include <QGroupBox>
#include <QCheckBox>



ImageTranslationWidget::ImageTranslationWidget(TranslationServiceManager *translationManager, QWidget *parent)
    : QWidget(parent)
    , m_translationManager(translationManager)
    , m_imageScene(new QGraphicsScene(this))
    , ui(new Ui::ImageTranslationWidget)
{
    ui->setupUi(this);
    
    // Setup Graphics View
    ui->m_imageView->setScene(m_imageScene);
    
    // Setup View Group (Logic only, buttons are in UI)
    m_viewGroup = new QButtonGroup(this);
    m_viewGroup->setExclusive(true);
    m_viewGroup->addButton(ui->m_btnViewOriginal, Original);
    m_viewGroup->addButton(ui->m_btnViewClean, Clean);
    m_viewGroup->addButton(ui->m_btnViewTranslated, Translated);
    
    connect(m_viewGroup, SIGNAL(idClicked(int)), this, SLOT(onViewModeChanged(int)));
    
    // Repopulate combo box to ensure user data is set (not possible in .ui safely)
    ui->m_comboSourceLang->clear();
    ui->m_comboSourceLang->addItem("English", "en");
    ui->m_comboSourceLang->addItem("Japanese", "ja");
    ui->m_comboSourceLang->addItem("Chinese", "ch_sim");
    ui->m_comboSourceLang->addItem("Korean", "ko");

    // --- GCV UI Setup (Programmatic) ---
    QGroupBox *gcvGroup = new QGroupBox("OCR Engine", this);
    QVBoxLayout *gcvLayout = new QVBoxLayout(gcvGroup);
    
    m_chkUseGcv = new QCheckBox("Use Google Cloud Vision", gcvGroup);
    m_chkUseGcv->setToolTip("Requires Service Account Key JSON");
    
    QHBoxLayout *keyLayout = new QHBoxLayout();
    m_editGcvKeyPath = new QLineEdit(gcvGroup);
    m_editGcvKeyPath->setPlaceholderText("Path to service-account.json");
    m_editGcvKeyPath->setEnabled(false);
    
    m_btnBrowseGcvKey = new QPushButton("...", gcvGroup);
    m_btnBrowseGcvKey->setFixedWidth(30);
    m_btnBrowseGcvKey->setEnabled(false);
    
    keyLayout->addWidget(m_editGcvKeyPath);
    keyLayout->addWidget(m_btnBrowseGcvKey);
    
    gcvLayout->addWidget(m_chkUseGcv);
    gcvLayout->addLayout(keyLayout);
    
    // Add to Side Panel (Assuming m_comboSourceLang is in the side panel layout)
    if (ui->m_comboSourceLang->parentWidget() && ui->m_comboSourceLang->parentWidget()->layout()) {
       ui->m_comboSourceLang->parentWidget()->layout()->addWidget(gcvGroup);
    }
    
    // GCV Logic
    connect(m_chkUseGcv, &QCheckBox::toggled, this, [this](bool checked){
        m_editGcvKeyPath->setEnabled(checked);
        m_btnBrowseGcvKey->setEnabled(checked);
    });
    
    connect(m_btnBrowseGcvKey, &QPushButton::clicked, this, [this](){
        QString path = QFileDialog::getOpenFileName(this, "Select GCV Key", "", "JSON (*.json)");
        if (!path.isEmpty()) {
            m_editGcvKeyPath->setText(path);
        }
    });
    
    // Connect UI signals
    connect(ui->m_btnLoad, &QPushButton::clicked, this, &ImageTranslationWidget::onAddImages);
    connect(ui->m_btnSave, &QPushButton::clicked, this, &ImageTranslationWidget::onSaveImage);
    connect(ui->m_btnTranslate, &QPushButton::clicked, this, &ImageTranslationWidget::onTranslate);
    connect(ui->m_btnTranslateAll, &QPushButton::clicked, this, &ImageTranslationWidget::onTranslateAll);
    connect(ui->m_btnStop, &QPushButton::clicked, this, &ImageTranslationWidget::onStopTranslation);
    
    connect(ui->m_btnRemove, &QPushButton::clicked, this, &ImageTranslationWidget::onRemoveImage);
    connect(ui->m_btnClear, &QPushButton::clicked, this, &ImageTranslationWidget::onClearAll);
    connect(ui->m_imageListWidget, &QListWidget::currentRowChanged, this, &ImageTranslationWidget::onImageSelected);
    
    connect(ui->m_btnPeekOriginal, &QPushButton::pressed, this, &ImageTranslationWidget::onPeekPressed);
    connect(ui->m_btnPeekOriginal, &QPushButton::released, this, &ImageTranslationWidget::onPeekReleased);
    
    connect(ui->m_chkDevMode, &QCheckBox::toggled, this, &ImageTranslationWidget::onDevModeToggled);
    
    // Connect to translation manager signals
    if (m_translationManager) {
        connect(m_translationManager, &TranslationServiceManager::translationFinished,
                this, &ImageTranslationWidget::onTranslationFinished);
        connect(m_translationManager, &TranslationServiceManager::errorOccurred,
                this, &ImageTranslationWidget::onTranslationError);
    }
    
    // Initialize Worker Thread
    m_workerThread = new QThread(this);
    m_worker = new ImageProcessorWorker();
    m_worker->moveToThread(m_workerThread);
    
    // Connect Worker Signals
    connect(m_workerThread, &QThread::started, m_worker, &ImageProcessorWorker::initialize);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    
    connect(this, &ImageTranslationWidget::processImageRequested, m_worker, &ImageProcessorWorker::processImage);
    
    connect(m_worker, &ImageProcessorWorker::initialized, this, &ImageTranslationWidget::onWorkerInitialized);
    connect(m_worker, &ImageProcessorWorker::progress, this, &ImageTranslationWidget::onWorkerProgress);
    connect(m_worker, &ImageProcessorWorker::processingFinished, this, &ImageTranslationWidget::onWorkerProcessingFinished);
    connect(m_worker, &ImageProcessorWorker::errorOccurred, this, &ImageTranslationWidget::onWorkerError);
    
    m_workerThread->start();
}

ImageTranslationWidget::~ImageTranslationWidget()
{
    m_workerThread->quit();
    m_workerThread->wait();
    delete ui;
}

void ImageTranslationWidget::setupUi()
{
    // Legacy setupUi removed. Logic moved to .ui file.
}

void ImageTranslationWidget::setSettings(const QString &apiKey, const QString &targetLanguage, bool googleApi,
                                          const QString &llmProvider, const QString &llmApiKey,
                                          const QString &llmModel, const QString &llmBaseUrl,
                                          int translationMode)
{
    m_apiKey = apiKey;
    m_targetLanguage = targetLanguage;
    m_googleApi = googleApi;
    m_llmProvider = llmProvider;
    m_llmApiKey = llmApiKey;
    m_llmModel = llmModel;
    m_llmBaseUrl = llmBaseUrl;
    m_translationMode = translationMode;
}


void ImageTranslationWidget::onAddImages()
{
    QStringList paths = QFileDialog::getOpenFileNames(this, "Add Images", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    
    for (const QString &path : paths) {
        // Avoid duplicates
        bool exists = false;
        for (const ImageItem &item : m_imageQueue) {
            if (item.path == path) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            ImageItem item;
            item.path = path;
            item.status = ImageItem::Pending;
            m_imageQueue.append(item);
            
            // Add to list widget with thumbnail
            QFileInfo fi(path);
            QListWidgetItem *listItem = new QListWidgetItem(fi.fileName());
            QPixmap thumb(path);
            if (!thumb.isNull()) {
                listItem->setIcon(QIcon(thumb.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            }
            listItem->setToolTip(path);
            ui->m_imageListWidget->addItem(listItem);
        }
    }
    
    // Auto-select first if none selected
    if (m_currentQueueIndex < 0 && !m_imageQueue.isEmpty()) {
        ui->m_imageListWidget->setCurrentRow(0);
    }
}

void ImageTranslationWidget::onRemoveImage()
{
    int row = ui->m_imageListWidget->currentRow();
    if (row >= 0 && row < m_imageQueue.size()) {
        m_imageQueue.removeAt(row);
        delete ui->m_imageListWidget->takeItem(row);
        
        if (m_imageQueue.isEmpty()) {
            m_currentQueueIndex = -1;
            m_imageScene->clear();
            m_currentImagePath.clear();
        } else if (row <= m_currentQueueIndex) {
            m_currentQueueIndex = qMax(0, m_currentQueueIndex - 1);
            ui->m_imageListWidget->setCurrentRow(m_currentQueueIndex);
        }
    }
}

void ImageTranslationWidget::onClearAll()
{
    m_imageQueue.clear();
    ui->m_imageListWidget->clear();
    m_currentQueueIndex = -1;
    m_imageScene->clear();
    m_currentImagePath.clear();
    m_detections = QJsonArray();
    m_translatedTexts.clear();
    m_inpaintedImagePath.clear();
    ui->m_statusLabel->setText("Ready.");
}

void ImageTranslationWidget::onImageSelected(int row)
{
    if (row < 0 || row >= m_imageQueue.size()) return;
    
    m_currentQueueIndex = row;
    const ImageItem &item = m_imageQueue[row];
    
    // Load image data from queue
    m_currentImagePath = item.path;
    m_detections = item.detections;
    m_translatedTexts = item.translatedTexts;
    m_inpaintedImagePath = item.inpaintedPath;
    
    // Update view
    if (item.status == ImageItem::Completed && !item.translatedTexts.isEmpty()) {
        m_currentViewMode = Translated;
        m_viewGroup->button(Translated)->setChecked(true);
    } else {
        m_currentViewMode = Original;
        m_viewGroup->button(Original)->setChecked(true);
    }
    
    updateViewMode();
    
    // Update status display
    QString statusStr;
    switch (item.status) {
        case ImageItem::Pending: statusStr = "Pending"; break;
        case ImageItem::Processing: statusStr = "Processing..."; break;
        case ImageItem::Completed: statusStr = "Completed"; break;
        case ImageItem::Error: statusStr = "Error"; break;
    }
    ui->m_statusLabel->setText(QString("Image: %1 [%2]").arg(QFileInfo(item.path).fileName(), statusStr));
}

void ImageTranslationWidget::updateListItemStatus(int index, int status)
{
    if (index < 0 || index >= ui->m_imageListWidget->count()) return;
    
    QListWidgetItem *item = ui->m_imageListWidget->item(index);
    QString emoji;
    switch (status) {
        case ImageItem::Pending: emoji = "🕐 "; break;
        case ImageItem::Processing: emoji = "⏳ "; break;
        case ImageItem::Completed: emoji = "✅ "; break;
        case ImageItem::Error: emoji = "❌ "; break;
    }
    
    QString fileName = QFileInfo(m_imageQueue[index].path).fileName();
    item->setText(emoji + fileName);
}

void ImageTranslationWidget::displayImage(const QString &path)
{
    m_currentImagePath = path;
    m_imageScene->clear();
    
    QPixmap pixmap(path);
    if (!pixmap.isNull()) {
        m_imageScene->addPixmap(pixmap);
        ui->m_imageView->setSceneRect(pixmap.rect());
        ui->m_imageView->fitInView(m_imageScene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }
}

bool ImageTranslationWidget::processImage(int index)
{
    if (index < 0 || index >= m_imageQueue.size()) return false;
    
    // We don't check d->translator here anymore, we check if worker is ready via status or assume it is
    // For now, let's assume if the thread is running, we can request
    
    ImageItem &item = m_imageQueue[index];
    QString imagePath = item.path;
    QString sourceLang = ui->m_comboSourceLang->currentData().toString();
    
    bool useGcv = m_chkUseGcv->isChecked();
    QString gcvKey = m_editGcvKeyPath->text();
    
    emit processImageRequested(imagePath, sourceLang, useGcv, gcvKey);
    return true; // Started async
}

void ImageTranslationWidget::onDevModeToggled(bool checked)
{
    ui->m_logConsole->setVisible(checked);
}

void ImageTranslationWidget::onWorkerInitialized(bool success, const QString &status, bool useGpu, const QString &deviceName)
{
    if (!success) {
        ui->m_statusLabel->setText("Status: " + status);
        ui->m_statusLabel->setStyleSheet("color: red;");
        ui->m_logConsole->append(QString("<font color='red'>[%1] Init Error: %2</font>").arg(QDateTime::currentDateTime().toString("HH:mm:ss")).arg(status));
        
        if (status.contains("Dependencies missing")) {
            QMessageBox::warning(this, "Missing Dependencies", 
                "The AI backend could not be initialized because some dependencies are missing.\n\n"
                "Please install the following packages using pip:\n"
                "pip install easyocr torch torchvision torchaudio simple-lama-inpainting\n\n"
                "Note: You may need to choose the correct version for your GPU/CPU."
            );
        }
    } else {
         if (useGpu) {
            ui->m_statusLabel->setText(QString("Status: Ready (%1)").arg(deviceName));
            ui->m_statusLabel->setStyleSheet("color: #00FF7F;"); 
            ui->m_logConsole->append(QString("<font color='#00FF7F'>[%1] Initialized GPU: %2</font>").arg(QDateTime::currentDateTime().toString("HH:mm:ss")).arg(deviceName));
        } else {
            ui->m_statusLabel->setText(QString("Status: Ready (CPU Mode) - %1").arg(status));
            ui->m_statusLabel->setStyleSheet("color: #FFD700;");
            ui->m_logConsole->append(QString("<font color='#FFD700'>[%1] Initialized CPU: %2</font>").arg(QDateTime::currentDateTime().toString("HH:mm:ss")).arg(status));
        }
    }
}

void ImageTranslationWidget::onWorkerProgress(const QString &message)
{
    int idx = m_currentQueueIndex;
    if (idx >= 0 && idx < m_imageQueue.size()) {
        ui->m_statusLabel->setText(QString("Processing %1/%2: %3").arg(idx + 1).arg(m_imageQueue.size()).arg(message));
    } else {
        ui->m_statusLabel->setText(message);
    }
    
    ui->m_logConsole->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss")).arg(message));
}

void ImageTranslationWidget::onWorkerProcessingFinished(const QString &imagePath, const QJsonArray &detections, const QString &inpaintedPath)
{
    // Find the item
    int idx = -1;
    for(int i=0; i<m_imageQueue.size(); ++i) {
        if(m_imageQueue[i].path == imagePath) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1) return;
    
    ImageItem &item = m_imageQueue[idx];
    item.detections = detections;
    item.inpaintedPath = inpaintedPath;
    
    // Continue to Translation Step (Main Thread)
    // Synchronous translation for now, but on main thread so it's safer for UI updates
    item.translatedTexts.clear();
    
    if (detections.isEmpty()) {
        item.status = ImageItem::Completed;
        updateListItemStatus(idx, m_imageQueue[idx].status);
        onImageSelected(idx); // Refresh
        
        // If batch, proceed
        if (m_isBatchProcessing && !m_cancelRequested) {
             // Trigger next one
             QTimer::singleShot(0, this, [this](){
                 // Find next pending
                 for(int i=0; i<m_imageQueue.size(); ++i) {
                     if (m_imageQueue[i].status == ImageItem::Pending) {
                         m_currentQueueIndex = i;
                         ui->m_imageListWidget->setCurrentRow(i);
                         m_imageQueue[i].status = ImageItem::Processing;
                         updateListItemStatus(i, ImageItem::Processing);
                         processImage(i);
                         return;
                     }
                 }
                 // All done
                 m_isBatchProcessing = false;
                 ui->m_btnTranslate->setEnabled(true);
                 ui->m_btnTranslateAll->setEnabled(true);
                 ui->m_btnStop->setVisible(false);
                 ui->m_statusLabel->setText("Batch Complete.");
             });
        } else {
            ui->m_statusLabel->setText("Finished (no text).");
            ui->m_btnTranslate->setEnabled(true);
            ui->m_btnTranslateAll->setEnabled(true);
            ui->m_btnStop->setVisible(false);
             ui->m_btnSave->setEnabled(true);
        }
        return;
    }

    QStringList textsToTranslate;
    for (const QJsonValue &val : detections) {
        textsToTranslate.append(val.toObject()["text"].toString());
    }
    
    QVariantMap settings;
    settings["targetLanguage"] = ui->m_editTargetLang->text();
    settings["googleApi"] = m_googleApi;
    settings["googleApiKey"] = m_apiKey;
    
    // Store context
    m_detections = detections;
    m_translatedTexts.clear();
    m_currentTranslationIndex = 0;
    m_inpaintedImagePath = inpaintedPath;
    
    // Determine Service Name based on explicit mode
    QString serviceName = "Google Translate";
    
    // Mode 2 is LLM
    if (m_translationMode == 2) {
        serviceName = "LLM Translation";
    } else {
        // Mode 0 (Quick) or 1 (Professional/GoogleAPI) -> Use Google Translate service
        // The service manager handles the difference between Free/API based on settings.
        serviceName = "Google Translate";
    }
    
    ui->m_statusLabel->setText("Translating...");
    m_translationManager->translate(serviceName, textsToTranslate, settings);
    
    ui->m_logConsole->append(QString("<font color='#00FF00'>[%1] Finished processing %2. found %3 detections.</font>").arg(QDateTime::currentDateTime().toString("HH:mm:ss")).arg(imagePath).arg(detections.size()));
}

void ImageTranslationWidget::onWorkerError(const QString &message)
{
    ui->m_statusLabel->setText("Error: " + message);
    if (m_currentQueueIndex >= 0) {
        m_imageQueue[m_currentQueueIndex].status = ImageItem::Error;
        updateListItemStatus(m_currentQueueIndex, ImageItem::Error);
    }
    
    ui->m_logConsole->append(QString("<font color='red'>[%1] Error: %2</font>").arg(QDateTime::currentDateTime().toString("HH:mm:ss")).arg(message));
    
    ui->m_btnTranslate->setEnabled(true);
    ui->m_btnTranslateAll->setEnabled(true);
    ui->m_btnStop->setVisible(false);
}

void ImageTranslationWidget::onTranslate()
{
    if (m_currentQueueIndex < 0 || m_currentQueueIndex >= m_imageQueue.size()) {
        QMessageBox::warning(this, "Warning", "Please select an image first.");
        return;
    }
    
    // We assume worker is initialized if buttons are enabled. 
    // Ideally we track m_backendReady but UI enabling logic handles most cases.
    
    m_cancelRequested = false;
    ui->m_btnTranslate->setEnabled(false);
    ui->m_btnTranslateAll->setEnabled(false);
    ui->m_btnStop->setVisible(true);
    ui->m_btnSave->setEnabled(false);
    
    int idx = m_currentQueueIndex;
    m_imageQueue[idx].status = ImageItem::Processing;
    updateListItemStatus(idx, ImageItem::Processing);
    
    // Async call
    if (!processImage(idx)) {
         m_imageQueue[idx].status = ImageItem::Error;
         updateListItemStatus(idx, ImageItem::Error);
         ui->m_btnTranslate->setEnabled(true);
         ui->m_btnTranslateAll->setEnabled(true);
         ui->m_btnStop->setVisible(false);
         ui->m_statusLabel->setText("Failed to start processing.");
         return;
    }
    
    // Returns immediately, results handled in slots
}

void ImageTranslationWidget::onTranslateAll()
{
    if (m_imageQueue.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please add images first.");
        return;
    }
    
    m_isBatchProcessing = true;
    m_cancelRequested = false;
    ui->m_btnTranslate->setEnabled(false);
    ui->m_btnTranslateAll->setEnabled(false);
    ui->m_btnStop->setVisible(true);
    
    // Find first pending and start
    for(int i=0; i<m_imageQueue.size(); ++i) {
        if (m_imageQueue[i].status == ImageItem::Pending) {
             m_currentQueueIndex = i;
             ui->m_imageListWidget->setCurrentRow(i);
             m_imageQueue[i].status = ImageItem::Processing;
             updateListItemStatus(i, ImageItem::Processing);
             processImage(i);
             return;
        }
    }
    
    // If nothing pending
    m_isBatchProcessing = false;
    ui->m_btnTranslate->setEnabled(true);
    ui->m_btnTranslateAll->setEnabled(true);
    ui->m_btnStop->setVisible(false);
    ui->m_statusLabel->setText("Nothing to translate.");
}


void ImageTranslationWidget::onStopTranslation()
{
    m_cancelRequested = true;
    ui->m_statusLabel->setText("Stopping...");
}

void ImageTranslationWidget::onTranslationFinished(const qtlingo::TranslationResult &result)
{
    m_translatedTexts.append(result.translatedText);
    m_currentTranslationIndex++;
    
    // Update queue persistence
    if (m_currentQueueIndex >= 0 && m_currentQueueIndex < m_imageQueue.size()) {
        m_imageQueue[m_currentQueueIndex].translatedTexts = m_translatedTexts;
    }
    
    // Guard against division by zero
    int total = m_detections.size();
    if (total <= 0) {
        // No detections - mark as finished
        ui->m_statusLabel->setText("Status: Finished (no text).");
        ui->m_btnTranslate->setEnabled(true);
        ui->m_btnTranslateAll->setEnabled(true);
        ui->m_btnSave->setEnabled(true);
        return;
    }
    
    int pct = (m_currentTranslationIndex * 100) / total;
    ui->m_statusLabel->setText(QString("Status: Translating... %1%").arg(pct));
    
    if (m_currentTranslationIndex >= total) {
        ui->m_statusLabel->setText("Status: Finished.");
        ui->m_btnTranslate->setEnabled(true);
        ui->m_btnTranslateAll->setEnabled(true);
        ui->m_btnSave->setEnabled(true);
        
        // Mark as completed
        if (m_currentQueueIndex >= 0 && m_currentQueueIndex < m_imageQueue.size()) {
             m_imageQueue[m_currentQueueIndex].status = ImageItem::Completed;
             updateListItemStatus(m_currentQueueIndex, ImageItem::Completed);
        }
        
        // Auto-switch to Translated View on finish
        m_currentViewMode = Translated;
        m_viewGroup->button(Translated)->setChecked(true);
        updateViewMode();
        
        // Continue Batch Processing
        if (m_isBatchProcessing && !m_cancelRequested) {
             // Find next pending
             bool foundNext = false;
             for(int i=0; i<m_imageQueue.size(); ++i) {
                if (m_imageQueue[i].status == ImageItem::Pending) {
                     m_currentQueueIndex = i;
                     // UI Update
                     ui->m_imageListWidget->setCurrentRow(i);
                     m_imageQueue[i].status = ImageItem::Processing;
                     updateListItemStatus(i, ImageItem::Processing);
                     
                     // Small delay to let UI refresh slightly or just process
                     QTimer::singleShot(100, this, [this, i](){
                         processImage(i);
                     });
                     foundNext = true;
                     return;
                }
             }
             
             if (!foundNext) {
                 m_isBatchProcessing = false;
                 ui->m_statusLabel->setText("Batch Processing Finished.");
                 ui->m_btnStop->setVisible(false);
             }
        }
    }
}

void ImageTranslationWidget::onTranslationError(const QString &message)
{
     ui->m_statusLabel->setText("Translation Error: " + message);
     ui->m_btnTranslate->setEnabled(true);
}

// Removed legacy: onDetectText, onOverlayModeChanged, drawDetections, drawOverlayMode

void ImageTranslationWidget::onSaveImage()
{
    if (m_currentImagePath.isEmpty()) return;
    
    // Save current content of graphics scene (what the user sees)
    QString savePath = QFileDialog::getSaveFileName(this, "Save Translation", "", "Images (*.png *.jpg)");
    if (!savePath.isEmpty()) {
        QImage image(m_imageScene->sceneRect().size().toSize(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        
        QPainter painter(&image);
        m_imageScene->render(&painter);
        
        image.save(savePath);
        ui->m_statusLabel->setText("Saved to: " + savePath);
    }
}

void ImageTranslationWidget::onViewModeChanged(int id)
{
    m_currentViewMode = static_cast<ViewMode>(id);
    updateViewMode();
}

void ImageTranslationWidget::onPeekPressed()
{
    // Temporarily show original
    m_imageScene->clear();
    m_imageScene->addPixmap(QPixmap(m_currentImagePath));
}

void ImageTranslationWidget::onPeekReleased()
{
    // Restore current view
    updateViewMode();
}

void ImageTranslationWidget::updateViewMode()
{
    m_imageScene->clear();
    
    if (m_currentImagePath.isEmpty()) return;
    
    QPixmap displayPixmap(m_currentImagePath); // Default
    
    if (m_currentViewMode == Clean || m_currentViewMode == Translated) {
        if (!m_inpaintedImagePath.isEmpty()) {
            displayPixmap = QPixmap(m_inpaintedImagePath);
        }
    }
    
    
    // Render Text if in Translated Mode
    if (m_currentViewMode == Translated && !m_translatedTexts.isEmpty()) {
        QPainter pixmapPainter(&displayPixmap);
        pixmapPainter.setRenderHint(QPainter::Antialiasing);
        pixmapPainter.setRenderHint(QPainter::TextAntialiasing);
        
        // --- TEXT RENDERING LOGIC (Moved from drawOverlayMode) ---
        for (int i = 0; i < m_detections.size(); ++i) {
            if (i >= m_translatedTexts.size()) break;
            
            QJsonObject obj = m_detections[i].toObject();
            QString translatedText = m_translatedTexts[i];
            QJsonArray bbox = obj["bbox"].toArray();
            bool isDark = obj["is_dark"].toBool();
            
            if (bbox.size() >= 4) {
                QJsonArray p1 = bbox[0].toArray();
                int x = p1[0].toInt();
                int y = p1[1].toInt();
                
                QJsonArray p3 = bbox[2].toArray();
                int w = p3[0].toInt() - x;
                int h = p3[1].toInt() - y;
                
                QRect textRect(x, y, w, h);
                
                int fontSize = h * 0.75;
                if (fontSize < 8) fontSize = 8;
                if (fontSize > 100) fontSize = 100;
                
                QFont font("Outfit", fontSize, QFont::Bold);
                
                QFontMetrics fm(font);
                while (fm.horizontalAdvance(translatedText) > w && fontSize > 8) {
                    fontSize--;
                    font.setPointSize(fontSize);
                    fm = QFontMetrics(font);
                }
                pixmapPainter.setFont(font);
                
                // Color Extraction Match
                QColor textColor = Qt::black;
                if (obj.contains("text_color")) {
                    QJsonArray rgb = obj["text_color"].toArray();
                    if (rgb.size() == 3) textColor = QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt());
                } else {
                    textColor = isDark ? Qt::white : Qt::black;
                }
                
                QColor haloColor = isDark ? QColor(0,0,0,150) : QColor(255,255,255,200);
                double angle = obj["angle"].toDouble();
                
                pixmapPainter.save();
                int centerX = x + w / 2;
                int centerY = y + h / 2;
                pixmapPainter.translate(centerX, centerY);
                pixmapPainter.rotate(angle);
                pixmapPainter.translate(-centerX, -centerY);
                
                // Calculate text position (centered in bbox)
                int textX = x + (w - fm.horizontalAdvance(translatedText)) / 2;
                int textY = y + (h + fm.ascent() - fm.descent()) / 2;
                
                // Multi-pass shadow/halo drawing technique
                // Draw text multiple times with offset positions for smooth halo effect
                // This avoids the "hollow" issue with QPainterPath stroke on complex glyphs
                
                int haloSize = 3;  // Halo thickness in pixels
                pixmapPainter.setPen(haloColor);
                
                // Draw halo in 8 directions + 4 diagonal midpoints for smooth coverage
                const int offsets[][2] = {
                    {-haloSize, 0}, {haloSize, 0}, {0, -haloSize}, {0, haloSize},  // 4 cardinal
                    {-haloSize, -haloSize}, {haloSize, -haloSize}, {-haloSize, haloSize}, {haloSize, haloSize},  // 4 diagonal
                    {-haloSize/2, -haloSize}, {haloSize/2, -haloSize}, {-haloSize/2, haloSize}, {haloSize/2, haloSize},  // 4 more for smoothness
                    {-haloSize, -haloSize/2}, {haloSize, -haloSize/2}, {-haloSize, haloSize/2}, {haloSize, haloSize/2}   // 4 more
                };
                
                for (const auto& offset : offsets) {
                    pixmapPainter.drawText(textX + offset[0], textY + offset[1], translatedText);
                }
                
                // Draw main text on top
                pixmapPainter.setPen(textColor);
                pixmapPainter.drawText(textX, textY, translatedText);
                
                pixmapPainter.restore();
            }
        }
        
    }
    
    m_imageScene->addPixmap(displayPixmap);
}
