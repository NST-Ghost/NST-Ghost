#include "imageprocessorworker.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <cstdlib>

#ifdef HAS_PYTHON
#if defined(slots)
#undef slots
#endif
#include <pybind11/embed.h>
#include <pybind11/stl.h>
namespace py = pybind11;

struct ImageProcessorWorker::Private {
    py::object translator;
};
#else
struct ImageProcessorWorker::Private {};
#endif

ImageProcessorWorker::ImageProcessorWorker(QObject *parent)
    : QObject(parent)
    , d(nullptr)
{
#ifdef HAS_PYTHON
    py::gil_scoped_acquire acquire;
#endif
    d = new Private;
}

ImageProcessorWorker::~ImageProcessorWorker()
{
#ifdef HAS_PYTHON
    // Must acquire GIL before destroying Python objects to avoid crash on exit
    py::gil_scoped_acquire acquire;
#endif
    delete d;
}

void ImageProcessorWorker::initialize()
{
#ifdef HAS_PYTHON
    try {
        // Ensure we acquire GIL for Python operations
        py::gil_scoped_acquire acquire;
        
        py::module_ sys = py::module_::import("sys");
        
        // Add paths for finding scripts module
        sys.attr("path").attr("append")(".");
        sys.attr("path").attr("append")("pylib");
        
        // Add APPDIR-based path for bundled distribution
        const char* appdir = std::getenv("APPDIR");
        if (appdir) {
            // Insert /usr/bin first so "import pylib" works (pylib is a subdir)
            std::string binPath = std::string(appdir) + "/usr/bin";
            sys.attr("path").attr("insert")(0, binPath);
        }
        
        py::module_ mod = py::module_::import("pylib.image_translator");
        
        d->translator = mod.attr("ImageTranslator")();
        
        // Get detailed device info
        py::dict deviceInfo = d->translator.attr("get_device_info")().cast<py::dict>();
        bool available = deviceInfo["available"].cast<bool>();
        bool useGpu = deviceInfo["use_gpu"].cast<bool>();
        QString deviceName = QString::fromStdString(deviceInfo["device_name"].cast<std::string>());
        QString gpuStatus = QString::fromStdString(deviceInfo["status"].cast<std::string>());
        
        emit initialized(available, gpuStatus, useGpu, deviceName);
        
    } catch (const std::exception &e) {
        emit initialized(false, QString("Error init Python: %1").arg(e.what()), false, "");
    } catch (...) {
        emit initialized(false, "Unknown error initializing Python backend", false, "");
    }
#else
    emit initialized(false, "Python support not available (built without HAS_PYTHON)", false, "");
#endif
}

void ImageProcessorWorker::processImage(const QString &imagePath, const QString &sourceLang, bool useGcv, const QString &gcvKeyPath)
{
#ifdef HAS_PYTHON
    QString fileName = QFileInfo(imagePath).fileName();
    
    try {
        // Ensure we acquire GIL for Python operations scope
        py::gil_scoped_acquire acquire;

        if (d->translator.is_none()) {
             emit errorOccurred("Worker not initialized");
             return;
        }

        // Step 1: OCR
        qDebug() << "Worker: Starting OCR for" << fileName;
        emit progress(QString("OCR... (Step 1/2) - %1").arg(fileName));
        
        std::string resInfo = d->translator.attr("translate_image")(
            imagePath.toStdString(),
            sourceLang.toStdString(),
            "th", // Target lang placeholder, not strictly used for OCR extraction
            true, // use_preprocessing
            0.3, // confidence_threshold
            useGcv,
            gcvKeyPath.toStdString()
        ).cast<std::string>();
        
        qDebug() << "Worker: OCR Finished. Parsing JSON...";
        
        QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(resInfo).toUtf8());
        if (!doc.isArray()) {
            if (doc.isObject() && doc.object().contains("error")) {
                 QString err = doc.object()["error"].toString();
                 qDebug() << "Worker: Python returned error:" << err;
                 emit errorOccurred(err);
                 return;
            }
            qDebug() << "Worker: Invalid OCR result format";
            emit errorOccurred("Invalid OCR result format");
            return;
        }
        QJsonArray detections = doc.array();
        qDebug() << "Worker: OCR found" << detections.size() << "regions.";
        
        if (detections.isEmpty()) {
            qDebug() << "Worker: No text found. Finishing.";
            // No text found, finish early
            emit processingFinished(imagePath, detections, ""); 
            return;
        }
        
        // Step 2: Inpainting
        qDebug() << "Worker: Starting Inpainting for" << fileName;
        emit progress(QString("Inpainting... (Step 2/2) - %1").arg(fileName));
        
        QString detectionsJson = QString::fromUtf8(QJsonDocument(detections).toJson(QJsonDocument::Compact));
        std::string enrichedJsonStr = d->translator.attr("inpaint_text_regions")(
            imagePath.toStdString(),
            detectionsJson.toStdString()
        ).cast<std::string>();
        
        qDebug() << "Worker: Inpainting Finished.";
        
        QString inpaintedPath;
        QJsonDocument enrichedDoc = QJsonDocument::fromJson(QString::fromStdString(enrichedJsonStr).toUtf8());
        if (enrichedDoc.isObject()) {
            QJsonObject root = enrichedDoc.object();
            inpaintedPath = root["inpainted_path"].toString();
            if (root.contains("detections")) {
                detections = root["detections"].toArray();
            }
        }
        
        qDebug() << "Worker: Processing complete for" << fileName;
        emit processingFinished(imagePath, detections, inpaintedPath);
        
    } catch (const std::exception &e) {
        emit errorOccurred(QString("Processing error: %1").arg(e.what()));
    } catch (...) {
        emit errorOccurred("Unknown error during processing");
    }
#else
    Q_UNUSED(imagePath);
    Q_UNUSED(sourceLang);
    Q_UNUSED(useGcv);
    Q_UNUSED(gcvKeyPath);
    emit errorOccurred("Python support not available (built without HAS_PYTHON)");
#endif
}
