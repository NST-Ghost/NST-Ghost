#include "llm_translation_service.h"
#include <QDebug>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace qtlingo {

LLMTranslationService::LLMTranslationService(QObject *parent)
    : ITranslationService(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &LLMTranslationService::onNetworkReply);
}

void LLMTranslationService::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void LLMTranslationService::setLlmProvider(const QString &provider)
{
    m_provider = provider;
}

void LLMTranslationService::setLlmModel(const QString &model)
{
    m_model = model;
}

void LLMTranslationService::setTargetLanguage(const QString &language)
{
    m_targetLanguage = language;
}

void LLMTranslationService::setSourceLanguage(const QString &language)
{
    m_sourceLanguage = language;
}

void LLMTranslationService::configure(const QVariantMap &settings)
{
    setApiKey(settings.value("llmApiKey").toString());
    m_provider = settings.value("llmProvider").toString();
    m_model = settings.value("llmModel").toString();
    setTargetLanguage(settings.value("targetLanguage").toString());
    setSourceLanguage(settings.value("sourceLanguage", "auto").toString());
}

void LLMTranslationService::translate(const QString &sourceText)
{
    m_currentSourceText = sourceText;

    if (m_apiKey.isEmpty() || m_provider.isEmpty() || m_model.isEmpty() || m_targetLanguage.isEmpty()) {
        emit errorOccurred("Missing required configuration for LLM translation (API Key, Provider, Model, or Target Language).");
        return;
    }

    QJsonObject requestBody;
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(30000); // 30 second timeout

    try {
        if (m_provider == "OpenAI") {
            buildOpenAIRequest(request, requestBody, sourceText);
        } else if (m_provider == "Anthropic") {
            buildAnthropicRequest(request, requestBody, sourceText);
        } else if (m_provider == "Google") {
            buildGoogleRequest(request, requestBody, sourceText);
        } else {
            emit errorOccurred("Unknown LLM provider: " + m_provider);
            return;
        }
    } catch (const std::exception& e) {
        emit errorOccurred(QString("Failed to build request: %1").arg(e.what()));
        return;
    }

    m_networkManager->post(request, QJsonDocument(requestBody).toJson());
}

void LLMTranslationService::onNetworkReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Network Error:" << reply->errorString();
        qDebug() << "Response:" << reply->readAll();
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    qDebug() << "LLM API Response:" << responseData;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonObj = jsonDoc.object();
    QString translatedText;

    try {
        if (m_provider == "OpenAI") {
            translatedText = parseOpenAIResponse(jsonObj);
        } else if (m_provider == "Anthropic") {
            translatedText = parseAnthropicResponse(jsonObj);
        } else if (m_provider == "Google") {
            translatedText = parseGoogleResponse(jsonObj);
        }
    } catch (const std::exception& e) {
        emit errorOccurred(QString("Failed to parse response: %1").arg(e.what()));
        reply->deleteLater();
        return;
    }

    if (translatedText.isEmpty() || translatedText.startsWith("[Error:")) {
        emit errorOccurred(translatedText.isEmpty() ? "[Error: Empty response from API]" : translatedText);
    } else {
        TranslationResult result;
        result.sourceText = m_currentSourceText;
        result.translatedText = translatedText;
        emit translationFinished(result);
    }

    reply->deleteLater();
}

void LLMTranslationService::buildOpenAIRequest(QNetworkRequest &request, QJsonObject &requestBody, const QString &sourceText)
{
    request.setUrl(QUrl("https://api.openai.com/v1/chat/completions"));
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    requestBody["model"] = m_model;
    QJsonArray messages;
    QJsonObject message;
    message["role"] = "user";
    QString sourceLangStr = (m_sourceLanguage.isEmpty() || m_sourceLanguage == "auto") ? "" : QString(" from %1").arg(m_sourceLanguage);
    message["content"] = QString("You are translating dialogue and UI text from a video game. Translate%1 to %2. Return ONLY the translated text using %2 script/characters. Do not mix writing systems. Do not add explanations.\n\n%3").arg(sourceLangStr, m_targetLanguage, sourceText);
    messages.append(message);
    requestBody["messages"] = messages;
}

void LLMTranslationService::buildAnthropicRequest(QNetworkRequest &request, QJsonObject &requestBody, const QString &sourceText)
{
    request.setUrl(QUrl("https://api.anthropic.com/v1/messages"));
    request.setRawHeader("x-api-key", m_apiKey.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");
    requestBody["model"] = m_model;
    requestBody["max_tokens"] = 1024;
    QJsonArray messages;
    QJsonObject message;
    message["role"] = "user";
    QString sourceLangStr = (m_sourceLanguage.isEmpty() || m_sourceLanguage == "auto") ? "" : QString(" from %1").arg(m_sourceLanguage);
    message["content"] = QString("You are translating dialogue and UI text from a video game. Translate%1 to %2. Return ONLY the translated text using %2 script/characters. Do not mix writing systems. Do not add explanations.\n\n%3").arg(sourceLangStr, m_targetLanguage, sourceText);
    messages.append(message);
    requestBody["messages"] = messages;
}

void LLMTranslationService::buildGoogleRequest(QNetworkRequest &request, QJsonObject &requestBody, const QString &sourceText)
{
    QUrl url(QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent").arg(m_model));
    QUrlQuery query;
    query.addQueryItem("key", m_apiKey);
    url.setQuery(query);
    request.setUrl(url);
    QJsonObject content;
    QJsonObject part;
    QString sourceLangStr = (m_sourceLanguage.isEmpty() || m_sourceLanguage == "auto") ? "" : QString(" from %1").arg(m_sourceLanguage);
    part["text"] = QString("You are translating dialogue and UI text from a video game. Translate%1 to %2. Return ONLY the translated text using %2 script/characters. Do not mix writing systems. Do not add explanations.\n\n%3").arg(sourceLangStr, m_targetLanguage, sourceText);
    QJsonArray parts;
    parts.append(part);
    content["parts"] = parts;
    QJsonArray contents;
    contents.append(content);
    requestBody["contents"] = contents;
}

QString LLMTranslationService::parseOpenAIResponse(const QJsonObject &jsonObj)
{
    if (jsonObj.contains("error")) {
        return "[Error: " + jsonObj["error"].toObject()["message"].toString() + "]";
    }
    QJsonArray choices = jsonObj["choices"].toArray();
    if (choices.isEmpty()) {
        return "[Error: No response from API]";
    }
    return choices[0].toObject()["message"].toObject()["content"].toString();
}

QString LLMTranslationService::parseAnthropicResponse(const QJsonObject &jsonObj)
{
    if (jsonObj.contains("error")) {
        return "[Error: " + jsonObj["error"].toObject()["message"].toString() + "]";
    }
    QJsonArray content = jsonObj["content"].toArray();
    if (content.isEmpty()) {
        return "[Error: No response from API]";
    }
    return content[0].toObject()["text"].toString();
}

QString LLMTranslationService::parseGoogleResponse(const QJsonObject &jsonObj)
{
    if (jsonObj.contains("error")) {
        return "[Error: " + jsonObj["error"].toObject()["message"].toString() + "]";
    }
    QJsonArray candidates = jsonObj["candidates"].toArray();
    if (candidates.isEmpty()) {
        return "[Error: No response from API]";
    }
    return candidates[0].toObject()["content"].toObject()["parts"].toArray()[0].toObject()["text"].toString();
}

} // namespace qtlingo