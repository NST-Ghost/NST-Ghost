#ifndef QTLINGO_LLM_TRANSLATION_SERVICE_H
#define QTLINGO_LLM_TRANSLATION_SERVICE_H

#include "qtlingo/translationservice.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QHash>
#include <QMap>

namespace qtlingo {

class QTLINGO_EXPORT LLMTranslationService : public ITranslationService {
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
    void configure(const TranslationSettings &settings) override;

    // Feature Package: Response Caching & Glossary
    void setResponseCacheEnabled(bool enabled);
    bool isResponseCacheEnabled() const;
    void clearResponseCache();

    void setGlossary(const QMap<QString, QString> &glossary);
    QMap<QString, QString> glossary() const;

private slots:
    void onNetworkReply(QNetworkReply *reply);

private:
    void buildChatCompletionRequest(QNetworkRequest &request, QJsonObject &requestBody, const QString &userContent, const QString &systemPrompt);
    void buildClaudeRequest(QNetworkRequest &request, QJsonObject &requestBody, const QString &userContent, const QString &systemPrompt);
    void buildGoogleRequest(QNetworkRequest &request, QJsonObject &requestBody, const QString &userContent, const QString &systemPrompt, bool vertex);

    QString parseChatCompletionResponse(const QJsonObject &jsonObj);
    QString parseClaudeResponse(const QJsonObject &jsonObj);
    QString parseGoogleResponse(const QJsonObject &jsonObj);
    
    QString buildSystemPrompt(const QString &glossaryContext) const;
    QString extractRelevantGlossary(const QString &sourceText) const;
    QString extractRelevantGlossary(const QStringList &sourceTexts) const;

    QString chatCompletionBaseUrl() const;
    QString chatCompletionEndpoint() const;
    bool isChatCompletionProvider() const;
    bool isClaudeProvider() const;
    bool isGoogleAiStudioProvider() const;
    bool isGoogleVertexProvider() const;

    QString makeCacheKey(const QString &sourceText) const;

    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    QString m_provider;
    QString m_model;
    QString m_baseUrl;
    QString m_targetLanguage;
    QString m_sourceLanguage = "auto";
    QString m_currentSourceText;

    // Glossary
    QMap<QString, QString> m_glossary;

    // Response Cache
    bool m_enableResponseCache = true;
    QHash<QString, QString> m_responseCache;

    // Batch tracking & Partial caching
    bool m_forceMaxCompletionTokens = false;
    bool m_isBatchMode = false;
    QStringList m_currentBatchTexts;
    QList<int> m_uncachedIndices;
    QMap<int, QString> m_batchCachedResults;
    QPointer<QNetworkReply> m_currentReply;
};

} // namespace qtlingo

#endif // QTLINGO_LLM_TRANSLATION_SERVICE_H
