#ifndef QTLINGO_GOOGLE_TRANSLATE_SERVICE_H
#define QTLINGO_GOOGLE_TRANSLATE_SERVICE_H

#include "qtlingo/translationservice.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace qtlingo {

class GoogleTranslateService : public ITranslationService {
    Q_OBJECT
public:
    GoogleTranslateService(QObject *parent = nullptr);
    QString serviceName() const override { return "Google Translate"; }
    void translate(const QString &sourceText) override;

    // Batch Translation
    bool supportsBatchTranslation() const override { return true; }
    void batchTranslate(const QStringList &sourceTexts) override;

    void setApiKey(const QString &apiKey) override;
    void setTargetLanguage(const QString &language) override;
    void setSourceLanguage(const QString &language);
    void setGoogleTranslateMode(bool isApi);
    void configure(const QVariantMap &settings) override;

private slots:
    void onNetworkReply(QNetworkReply *reply);

private:
    // RPG Maker Code Protection
    QString preprocessText(const QString &text, QMap<QString, QString> &map);
    QString postprocessText(const QString &text, const QMap<QString, QString> &map);

    QString extractTranslationFromHtml(const QString &html);

    struct RequestData {
        bool isApi;
        bool isBatch;
        int batchIndex = -1;          // index in concurrent pool (-1 = not pool)
        QString sourceText;           // For single
        QStringList batchSourceTexts; // For API batch
        QMap<QString, QString> tagMap;           // For single
        QList<QMap<QString, QString>> batchTagMaps; // For API batch
    };

    // Concurrent pool for free mode batch translation
    struct BatchState {
        QStringList sourceTexts;
        QList<QMap<QString, QString>> tagMaps;
        QList<TranslationResult> results;
        int totalCount = 0;
        int completedCount = 0;
        int nextIndex = 0;
        static constexpr int POOL_SIZE = 5;
    };
    BatchState m_batchState;

    void dispatchNextFreeRequests();

    QMap<QNetworkReply*, RequestData> m_activeRequests;

    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    QString m_targetLanguage;
    QString m_sourceLanguage = "auto"; // Default to auto-detect
    bool m_isApi = false;
};

} // namespace qtlingo

#endif // QTLINGO_GOOGLE_TRANSLATE_SERVICE_H
