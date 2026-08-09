#include "translationservicemanager.h"
#include <QDebug>
#include <QThread>

TranslationServiceManager::TranslationServiceManager(QObject *parent)
    : QObject(parent)
{
    m_processTimer.setSingleShot(true);
    connect(&m_processTimer, &QTimer::timeout, this, &TranslationServiceManager::processNextTranslation);
}

TranslationServiceManager::~TranslationServiceManager()
{
    cancel();
}

QStringList TranslationServiceManager::getAvailableServices() const
{
    return qtlingo::availableTranslationServices();
}

void TranslationServiceManager::translate(const QString &serviceName, const QStringList &sourceTexts, const TranslationSettings &settings)
{
    if (sourceTexts.isEmpty()) return;

    cancel(); // Clean up existing workers

    m_currentServiceName = serviceName;
    m_totalItems = sourceTexts.size();
    m_processedItems = 0;

    QSettings qsettings;
    m_maxParallelWorkers = qsettings.value("Translation/ParallelWorkers", 2).toInt();
    if (m_maxParallelWorkers < 1) m_maxParallelWorkers = 1;
    if (m_maxParallelWorkers > 4) m_maxParallelWorkers = 4;

    emit logMessage(QString("[INFO] Initializing Parallel Pipeline: %1 Workers for service '%2'...")
                    .arg(m_maxParallelWorkers).arg(serviceName));

    // Create parallel worker slots
    for (int i = 0; i < m_maxParallelWorkers; ++i) {
        WorkerSlot *worker = new WorkerSlot();
        worker->id = i;
        worker->service = qtlingo::createTranslationService(serviceName, this).release();

        if (!worker->service) {
            emit errorOccurred(QString("Failed to create translation service: %1").arg(serviceName));
            delete worker;
            cancel();
            return;
        }

        connect(worker->service, &qtlingo::ITranslationService::translationFinished, this, [this, worker](const qtlingo::TranslationResult &res) {
            onWorkerDone(worker, res);
        });

        connect(worker->service, &qtlingo::ITranslationService::batchTranslationFinished, this, [this, worker](const QList<qtlingo::TranslationResult> &results) {
            onWorkerBatchDone(worker, results);
        });

        connect(worker->service, &qtlingo::ITranslationService::errorOccurred, this, [this, worker](const QString &msg) {
            onWorkerError(worker, msg);
        });

        connect(worker->service, &qtlingo::ITranslationService::logMessage, this, [this, worker](const QString &msg) {
            emit logMessage(QString("[Worker-%1] %2").arg(worker->id + 1).arg(msg));
        });

        worker->service->configure(settings);
        m_workers.append(worker);
    }

    // Load queue
    m_translationQueue.clear();
    for (const QString &text : sourceTexts) {
        m_translationQueue.enqueue(text);
    }

    m_processTimer.start(0);
}

void TranslationServiceManager::cancel()
{
    m_processTimer.stop();
    m_translationQueue.clear();

    for (WorkerSlot *worker : m_workers) {
        if (worker) {
            if (worker->service) {
                disconnect(worker->service, nullptr, this, nullptr);
                worker->service->deleteLater();
            }
            delete worker;
        }
    }
    m_workers.clear();
}

void TranslationServiceManager::processNextTranslation()
{
    if (m_workers.isEmpty()) return;

    bool allIdle = true;

    for (WorkerSlot *worker : m_workers) {
        if (worker->isProcessing) {
            allIdle = false;
            continue;
        }

        if (m_translationQueue.isEmpty()) continue;

        // Worker is idle and queue has items -> dispatch new batch!
        worker->isProcessing = true;
        worker->currentBatch.clear();

        if (worker->service->supportsBatchTranslation()) {
            QSettings qsettings;
            int maxItems = qsettings.value("Translation/BatchSize", 25).toInt();
            int maxChars = qsettings.value("Translation/BatchMaxChars", 4000).toInt();
            if (maxItems < 1) maxItems = 25;
            if (maxChars < 500) maxChars = 4000;

            int currentChars = 0;

            while (!m_translationQueue.isEmpty() && worker->currentBatch.size() < maxItems) {
                QString text = m_translationQueue.head();
                if (currentChars + text.length() > maxChars && !worker->currentBatch.isEmpty()) {
                    break;
                }
                worker->currentBatch.append(m_translationQueue.dequeue());
                currentChars += text.length();
            }

            emit logMessage(QString("[WORKER-%1] Dispatched batch of %2 items (%3 chars)")
                            .arg(worker->id + 1).arg(worker->currentBatch.size()).arg(currentChars));
            worker->service->batchTranslate(worker->currentBatch);
        } else {
            worker->currentBatch.append(m_translationQueue.dequeue());
            worker->service->translate(worker->currentBatch.first());
        }

        allIdle = false;
    }

    if (allIdle && m_translationQueue.isEmpty()) {
        emit progressUpdated(m_totalItems, m_totalItems);
        emit logMessage("[SUCCESS] All Parallel Workers Completed Translation Pipeline!");
    }
}

void TranslationServiceManager::onWorkerBatchDone(WorkerSlot *worker, const QList<qtlingo::TranslationResult> &results)
{
    if (!worker) return;

    emit logMessage(QString("[WORKER-%1] Completed batch of %2 items").arg(worker->id + 1).arg(results.size()));

    worker->retryCount = 0;
    worker->currentBatch.clear();

    for (const auto &result : results) {
        emit translationFinished(result);
        m_processedItems++;
    }

    emit progressUpdated(m_processedItems, m_totalItems);
    worker->isProcessing = false;

    // Trigger immediate scheduling for this worker
    m_processTimer.start(10);
}

void TranslationServiceManager::onWorkerDone(WorkerSlot *worker, const qtlingo::TranslationResult &result)
{
    if (!worker) return;

    worker->retryCount = 0;
    worker->currentBatch.clear();

    emit translationFinished(result);
    m_processedItems++;
    emit progressUpdated(m_processedItems, m_totalItems);

    worker->isProcessing = false;

    m_processTimer.start(10);
}

void TranslationServiceManager::onWorkerError(WorkerSlot *worker, const QString &message)
{
    if (!worker) return;

    emit logMessage(QString("[WARN] [Worker-%1] Error encountered: %2").arg(worker->id + 1).arg(message));

    // Re-queue uncompleted batch items so they are not lost
    if (!worker->currentBatch.isEmpty()) {
        emit logMessage(QString("[RETRY] [Worker-%1] Re-queuing %2 failed items for retry...")
                        .arg(worker->id + 1).arg(worker->currentBatch.size()));
        for (int i = worker->currentBatch.size() - 1; i >= 0; --i) {
            m_translationQueue.prepend(worker->currentBatch.at(i));
        }
        worker->currentBatch.clear();
    }

    worker->isProcessing = false;

    // Delay slightly before retrying
    QTimer::singleShot(2000, this, &TranslationServiceManager::processNextTranslation);
}
