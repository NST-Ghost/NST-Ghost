#include "translationservicemanager.h"
#include <QDebug>
#include <QSettings>

TranslationServiceManager::TranslationServiceManager(QObject *parent)
    : QObject(parent)
{
    m_processTimer.setSingleShot(true);
    connect(&m_processTimer, &QTimer::timeout, this, &TranslationServiceManager::processNextTranslation);
}

TranslationServiceManager::~TranslationServiceManager()
{
    qDeleteAll(m_services);
}

QStringList TranslationServiceManager::getAvailableServices() const
{
    return qtlingo::availableTranslationServices();
}

void TranslationServiceManager::translate(const QString &serviceName, const QStringList &sourceTexts, const QVariantMap &settings)
{
    if (sourceTexts.isEmpty()) return;

    // Stop any ongoing processing before starting a new one.
    if (m_isProcessing) {
        m_processTimer.stop();
        m_isProcessing = false;
    }

    m_currentServiceName = serviceName;
    m_currentService = qtlingo::createTranslationService(m_currentServiceName, nullptr).release();
    if (!m_currentService) {
        emit errorOccurred(QString("Failed to create translation service: %1").arg(m_currentServiceName));
        return;
    }

    // Disconnect previous connections if any
    if (m_services.size() > 0) {
        for(auto service : m_services) {
            disconnect(service, nullptr, this, nullptr);
            service->deleteLater();
        }
        m_services.clear();
    }

    connect(m_currentService, &qtlingo::ITranslationService::translationFinished, this, &TranslationServiceManager::onTranslationDone);
    connect(m_currentService, &qtlingo::ITranslationService::batchTranslationFinished, this, &TranslationServiceManager::onBatchTranslationDone);
    connect(m_currentService, &qtlingo::ITranslationService::errorOccurred, this, &TranslationServiceManager::onTranslationError);

    if (m_currentServiceName == "Google Translate") {
        m_currentService->setTargetLanguage(settings.value("targetLanguage").toString());
        m_currentService->setSourceLanguage(settings.value("sourceLanguage", "auto").toString());
        m_currentService->setGoogleTranslateMode(settings.value("googleApi").toBool());
        if (settings.value("googleApi").toBool()) {
            m_currentService->setApiKey(settings.value("googleApiKey").toString());
        }
    } else if (m_currentServiceName == "LLM Translation") {
        m_currentService->setLlmProvider(settings.value("llmProvider").toString());
        m_currentService->setApiKey(settings.value("llmApiKey").toString());
        m_currentService->setLlmModel(settings.value("llmModel").toString());
        m_currentService->setTargetLanguage(settings.value("targetLanguage").toString());
        m_currentService->setSourceLanguage(settings.value("sourceLanguage", "auto").toString());
    }

    m_totalItems = sourceTexts.size();
    m_processedItems = 0;
    
    // Load persisted delay for this service
    QSettings qsettings("MySoft", "NST");
    m_currentDelay = qsettings.value("ServiceDelays/" + m_currentServiceName, 0).toInt();

    m_services.append(m_currentService);
    
    // Populate queue
    m_translationQueue.clear();
    for (const QString &text : sourceTexts) {
        m_translationQueue.enqueue(text);
    }
    
    // Always start with processNextTranslation, which will handle batching if supported
    processNextTranslation();
}

void TranslationServiceManager::onBatchTranslationDone(const QList<qtlingo::TranslationResult> &results)
{
    // Remove processed items from queue
    // We assume the results correspond to the requested batch (or at least the attempt finished)
    // Even if results are fewer (partial failure?), we should probably dequeue the batch size
    // to avoid infinite loops, or rely on m_currentBatch size.
    
    int count = m_currentBatch.size();
    for(int i=0; i<count; ++i) {
        if (!m_translationQueue.isEmpty()) m_translationQueue.dequeue();
    }
    m_currentBatch.clear();

    for (const auto &result : results) {
        emit translationFinished(result);
        m_processedItems++;
    }
    emit progressUpdated(m_processedItems, m_totalItems);
    
    // Gradually decrease delay on success
    m_currentDelay = qMax(0, m_currentDelay - m_delayStep / 5);
    QSettings settings("MySoft", "NST");
    settings.setValue("ServiceDelays/" + m_currentServiceName, m_currentDelay);

    m_isProcessing = false;
    
    // Schedule the next batch
    if (!m_translationQueue.isEmpty()) {
        m_processTimer.start(m_currentDelay);
    }
}

void TranslationServiceManager::processNextTranslation()
{
    if (m_translationQueue.isEmpty() || m_isProcessing) {
        if(m_translationQueue.isEmpty()) {
            emit progressUpdated(m_totalItems, m_totalItems);
        }
        return;
    }

    m_isProcessing = true;

    if (m_currentService->supportsBatchTranslation()) {
        // Batch mode: Take up to 30 items
        int batchSize = 30; 
        m_currentBatch.clear();
        
        for (int i = 0; i < batchSize && i < m_translationQueue.size(); ++i) {
            m_currentBatch.append(m_translationQueue.at(i));
        }
        
        m_currentService->batchTranslate(m_currentBatch);
    } else {
        // Single mode
        QString text = m_translationQueue.head(); 
        m_currentService->translate(text);
    }
}

void TranslationServiceManager::onTranslationDone(const qtlingo::TranslationResult &result)
{
    if (!m_translationQueue.isEmpty()) {
        m_translationQueue.dequeue(); // Remove the successfully processed item
    }

    // Gradually decrease delay on success
    m_currentDelay = qMax(0, m_currentDelay - m_delayStep / 5);
    
    // Persist the new delay
    QSettings settings("MySoft", "NST");
    settings.setValue("ServiceDelays/" + m_currentServiceName, m_currentDelay);

    emit translationFinished(result);
    m_processedItems++;
    emit progressUpdated(m_processedItems, m_totalItems);
    
    m_isProcessing = false;
    // Schedule the next translation
    if (!m_translationQueue.isEmpty()) {
        m_processTimer.start(m_currentDelay);
    }
}

void TranslationServiceManager::onTranslationError(const QString &message)
{
    // Increase delay on error
    m_currentDelay = qMin(m_maxDelay, m_currentDelay + m_delayStep);

    // Persist the new delay
    QSettings settings("MySoft", "NST");
    settings.setValue("ServiceDelays/" + m_currentServiceName, m_currentDelay);
    
    // We don't dequeue, so the failed item remains at the head
    emit errorOccurred(message);
    
    m_isProcessing = false;
    // Schedule a retry after the new delay
    if (!m_translationQueue.isEmpty()) {
        m_processTimer.start(m_currentDelay);
    }
}
