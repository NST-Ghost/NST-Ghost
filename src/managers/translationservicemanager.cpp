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
    delete m_currentService;
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

    // Reuse existing service if same type, otherwise create new one
    if (!m_currentService || m_currentService->serviceName() != serviceName) {
        // Cleanup old service
        if (m_currentService) {
            disconnect(m_currentService, nullptr, this, nullptr);
            m_currentService->deleteLater();
            m_currentService = nullptr;
        }

        // Create new service with 'this' as parent for proper memory management
        m_currentService = qtlingo::createTranslationService(serviceName, this).release();
        if (!m_currentService) {
            emit errorOccurred(QString("Failed to create translation service: %1").arg(serviceName));
            return;
        }

        connect(m_currentService, &qtlingo::ITranslationService::translationFinished, this, &TranslationServiceManager::onTranslationDone);
        connect(m_currentService, &qtlingo::ITranslationService::batchTranslationFinished, this, &TranslationServiceManager::onBatchTranslationDone);
        connect(m_currentService, &qtlingo::ITranslationService::errorOccurred, this, &TranslationServiceManager::onTranslationError);
    }

    // Always reconfigure with latest settings
    m_currentService->configure(settings);

    m_totalItems = sourceTexts.size();
    m_processedItems = 0;

    // Load persisted delay for this service
    QSettings qsettings("MySoft", "NST");
    m_currentDelay = qsettings.value("ServiceDelays/" + m_currentServiceName, 0).toInt();
    
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
    m_retryCount = 0;

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
        // Build a batch based on both max item count and max total string length
        // to prevent LLMs from truncating huge prompts.
        int maxItems = 20; 
        int maxChars = 2000; // Roughly 500-1000 tokens of input
        int currentChars = 0;
        
        m_currentBatch.clear();
        
        for (int i = 0; i < m_translationQueue.size() && m_currentBatch.size() < maxItems; ++i) {
            QString text = m_translationQueue.at(i);
            if (currentChars + text.length() > maxChars && !m_currentBatch.isEmpty()) {
                break; // Stop adding if we exceed the approximate char limit, unless it's the first item
            }
            m_currentBatch.append(text);
            currentChars += text.length();
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
    m_retryCount = 0;

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
    
    emit errorOccurred(message);
    
    m_retryCount++;
    if (m_retryCount >= 3) {
        emit errorOccurred("Max retries reached. Skipping this translation block to avoid infinite loop.");
        
        if (m_currentService->supportsBatchTranslation()) {
            int count = m_currentBatch.size();
            for(int i=0; i<count; ++i) {
                if (!m_translationQueue.isEmpty()) m_translationQueue.dequeue();
                m_processedItems++; // Count as processed so the progress bar moves forward
            }
            m_currentBatch.clear();
        } else {
            if (!m_translationQueue.isEmpty()) m_translationQueue.dequeue();
            m_processedItems++;
        }
        
        emit progressUpdated(m_processedItems, m_totalItems);
        m_retryCount = 0; // Reset for the next item
    } else {
        emit errorOccurred(QString("Retrying (Attempt %1 of 3)...").arg(m_retryCount));
    }
    
    m_isProcessing = false;
    // Schedule a retry (or start the next block) after the new delay
    if (!m_translationQueue.isEmpty()) {
        m_processTimer.start(m_currentDelay);
    }
}
