#ifndef TRANSLATIONSERVICEMANAGER_H
#define TRANSLATIONSERVICEMANAGER_H

#include <QObject>
#include <QStringList>
#include <QList>
#include <QPointer>
#include <QTimer>
#include <QQueue>
#include <QSettings>
#include <qtlingo/translationservice.h>
#include <qtlingo/translationservicefactory.h>
#include <QVariantMap>

struct WorkerSlot {
    int id = 0;
    qtlingo::ITranslationService *service = nullptr;
    bool isProcessing = false;
    QStringList currentBatch;
    int retryCount = 0;
};

class TranslationServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit TranslationServiceManager(QObject *parent = nullptr);
    ~TranslationServiceManager();

    QStringList getAvailableServices() const;
    void translate(const QString &serviceName, const QStringList &sourceTexts, const TranslationSettings &settings);
    void cancel();

signals:
    void translationFinished(const qtlingo::TranslationResult &result);
    void errorOccurred(const QString &message);
    void progressUpdated(int current, int total);
    void logMessage(const QString &message);

private slots:
    void processNextTranslation();
    void onWorkerDone(WorkerSlot *worker, const qtlingo::TranslationResult &result);
    void onWorkerBatchDone(WorkerSlot *worker, const QList<qtlingo::TranslationResult> &results);
    void onWorkerError(WorkerSlot *worker, const QString &message);

private:
    QQueue<QString> m_translationQueue;
    QList<WorkerSlot*> m_workers;
    QString m_currentServiceName;
    int m_totalItems = 0;
    int m_processedItems = 0;
    int m_maxParallelWorkers = 2; // Default 2 parallel workers for PAYG
    QTimer m_processTimer;
};

#endif // TRANSLATIONSERVICEMANAGER_H
