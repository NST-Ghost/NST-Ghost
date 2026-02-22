#ifndef TRANSLATIONSERVICEMANAGER_H
#define TRANSLATIONSERVICEMANAGER_H

#include <QObject>
#include <QStringList>
#include <QList>
#include <QPointer>
#include <QTimer>
#include <QQueue>
#include <qtlingo/translationservice.h>
#include <qtlingo/translationservicefactory.h>
#include <QVariantMap>

class TranslationServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit TranslationServiceManager(QObject *parent = nullptr);
    ~TranslationServiceManager();

    QStringList getAvailableServices() const;
    void translate(const QString &serviceName, const QStringList &sourceTexts, const QVariantMap &settings);

signals:
    void translationFinished(const qtlingo::TranslationResult &result);
    void errorOccurred(const QString &message);
    void progressUpdated(int current, int total);

private slots:
    void processNextTranslation();
    void onTranslationDone(const qtlingo::TranslationResult &result);
    void onBatchTranslationDone(const QList<qtlingo::TranslationResult> &results);
    void onTranslationError(const QString &message);

private:
    QQueue<QString> m_translationQueue;
    QStringList m_currentBatch; // Track current batch for error handling
    qtlingo::ITranslationService* m_currentService = nullptr;
    QString m_currentServiceName;
    int m_totalItems = 0;
    int m_processedItems = 0;

    QTimer m_processTimer;
    int m_currentDelay = 0;
    const int m_maxDelay = 5000; // 5 seconds
    const int m_delayStep = 500;  // 500 ms
    bool m_isProcessing = false;
    int m_retryCount = 0;
};

#endif // TRANSLATIONSERVICEMANAGER_H
