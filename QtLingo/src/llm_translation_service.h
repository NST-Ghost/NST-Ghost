#ifndef QTLINGO_LLM_TRANSLATION_SERVICE_H
#define QTLINGO_LLM_TRANSLATION_SERVICE_H

#include "qtlingo/translationservice.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

namespace qtlingo {

class LLMTranslationService : public ITranslationService {
    Q_OBJECT
public:
    LLMTranslationService(QObject *parent = nullptr);
    QString serviceName() const override { return "LLM Translation"; }
    void translate(const QString &sourceText) override;
    
    bool supportsBatchTranslation() const override { return true; }
    void batchTranslate(const QStringList &sourceTexts) override;

    void setApiKey(const QString &apiKey) override;
    void setLlmProvider(const QString &provider);
    void setLlmModel(const QString &model);
    void setLlmBaseUrl(const QString &baseUrl);
    void setTargetLanguage(const QString &language) override;
    void setSourceLanguage(const QString &language) override;
    void configure(const QVariantMap &settings) override;

private slots:
    void onNetworkReply(QNetworkReply *reply);

private:
    void buildOpenAIRequest(QNetworkRequest &request, QJsonObject &requestBody, const QString &sourceText);
    void buildAnthropicRequest(QNetworkRequest &request, QJsonObject &requestBody, const QString &sourceText);
    void buildGoogleRequest(QNetworkRequest &request, QJsonObject &requestBody, const QString &sourceText);

    QString parseOpenAIResponse(const QJsonObject &jsonObj);
    QString parseAnthropicResponse(const QJsonObject &jsonObj);
    QString parseGoogleResponse(const QJsonObject &jsonObj);

    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    QString m_provider;
    QString m_model;
    QString m_targetLanguage;
    QString m_sourceLanguage = "auto";
    QString m_currentSourceText;

    // Batch tracking
    bool m_forceMaxCompletionTokens = false;
    bool m_isBatchMode = false;
    QStringList m_currentBatchTexts;
    QPointer<QNetworkReply> m_currentReply;
};

} // namespace qtlingo

#endif // QTLINGO_LLM_TRANSLATION_SERVICE_H
