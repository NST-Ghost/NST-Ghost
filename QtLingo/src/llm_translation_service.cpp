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
    m_isBatchMode = false;
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

void LLMTranslationService::batchTranslate(const QStringList &sourceTexts)
{
    m_isBatchMode = true;
    m_currentBatchTexts = sourceTexts;
    
    if (m_apiKey.isEmpty() || m_provider.isEmpty() || m_model.isEmpty() || m_targetLanguage.isEmpty()) {
        emit errorOccurred("Missing required configuration for LLM translation (API Key, Provider, Model, or Target Language).");
        return;
    }

    // Convert list to JSON string for the prompt
    QJsonArray sourceArray;
    for (const QString &text : sourceTexts) {
        sourceArray.append(text);
    }
    QString sourceJsonStr = QString::fromUtf8(QJsonDocument(sourceArray).toJson(QJsonDocument::Compact));

    QJsonObject requestBody;
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(60000); // 60 seconds for batches

    try {
        if (m_provider == "OpenAI") {
            buildOpenAIRequest(request, requestBody, sourceJsonStr);
        } else if (m_provider == "Anthropic") {
            buildAnthropicRequest(request, requestBody, sourceJsonStr);
        } else if (m_provider == "Google") {
            buildGoogleRequest(request, requestBody, sourceJsonStr);
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
    QByteArray responseData = reply->readAll();
    
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Network Error:" << reply->errorString();
        qDebug() << "Response:" << responseData;
        
        QString errorMsg = reply->errorString();
        QJsonDocument errDoc = QJsonDocument::fromJson(responseData);
        if (errDoc.isObject() && errDoc.object().contains("error")) {
             QJsonObject errObj = errDoc.object()["error"].toObject();
             if (errObj.contains("message")) {
                 errorMsg += "\nDetails: " + errObj["message"].toString();
             }
        } else if (!responseData.isEmpty()) {
             errorMsg += "\nDetails: " + QString::fromUtf8(responseData).left(200);
        }
        
        emit errorOccurred(errorMsg);
        reply->deleteLater();
        return;
    }

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

    if (!m_isBatchMode) {
        if (translatedText.isEmpty() || translatedText.startsWith("[Error:")) {
            emit errorOccurred(translatedText.isEmpty() ? "[Error: Empty response from API]" : translatedText);
        } else {
            TranslationResult result;
            result.sourceText = m_currentSourceText;
            result.translatedText = translatedText;
            emit translationFinished(result);
        }
    } else {
        if (translatedText.isEmpty() || translatedText.startsWith("[Error:")) {
            emit errorOccurred(translatedText.isEmpty() ? "[Error: Empty response from API]" : translatedText);
        } else {
            // Strip any potential markdown formatting the LLM might have added around the JSON
            QString cleanJsonStr = translatedText.trimmed();
            if (cleanJsonStr.startsWith("```json")) {
                cleanJsonStr.remove(0, 7);
            } else if (cleanJsonStr.startsWith("```")) {
                cleanJsonStr.remove(0, 3);
            }
            if (cleanJsonStr.endsWith("```")) {
                cleanJsonStr.chop(3);
            }
            cleanJsonStr = cleanJsonStr.trimmed();

            QJsonParseError parseError;
            QJsonDocument resultDoc = QJsonDocument::fromJson(cleanJsonStr.toUtf8(), &parseError);
            
            if (parseError.error != QJsonParseError::NoError || !resultDoc.isArray()) {
                emit errorOccurred(QString("Failed to parse batch JSON response: %1").arg(parseError.errorString()));
            } else {
                QJsonArray resultArray = resultDoc.array();
                QList<TranslationResult> results;
                
                // Salvage whatever we can rather than rejecting the whole batch and looping infinitely
                int parseCount = qMin(resultArray.size(), m_currentBatchTexts.size());
                
                for (int i = 0; i < parseCount; ++i) {
                    TranslationResult res;
                    res.sourceText = m_currentBatchTexts.at(i);
                    res.translatedText = resultArray.at(i).toString();
                    results.append(res);
                }
                
                if (resultArray.size() != m_currentBatchTexts.size()) {
                    // We still emit the salvaged results, but also fire a warning so the user knows
                    emit errorOccurred(QString("Warning: Batch skip detected. LLM translated %1 items instead of %2.")
                                        .arg(resultArray.size()).arg(m_currentBatchTexts.size()));
                }
                
                emit batchTranslationFinished(results);
            }
        }
    }

    reply->deleteLater();
}

void LLMTranslationService::buildOpenAIRequest(QNetworkRequest &request, QJsonObject &requestBody, const QString &sourceText)
{
    request.setUrl(QUrl("https://api.openai.com/v1/chat/completions"));
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    requestBody["model"] = m_model;
    
    // Note: o1, o3, gpt-4o, and gpt-5 models require/prefer max_completion_tokens instead of max_tokens
    if (m_model.startsWith("o1") || m_model.startsWith("o3") || 
        m_model.contains("gpt-4o") || m_model.contains("gpt-5") || m_model.contains("mini")) {
        requestBody["max_completion_tokens"] = m_isBatchMode ? 16384 : 4096;
    } else {
        requestBody["max_tokens"] = m_isBatchMode ? 4096 : 2048;
    }
    
    // response_format forces gpt-4o/gpt-4-turbo to return JSON
    if (m_isBatchMode && (m_model.contains("gpt-4o") || m_model.contains("gpt-4-turbo") || m_model.contains("gpt-3.5"))) {
        // o1 models might not support this directly yet
        QJsonObject format;
        format["type"] = "json_object";
        // requestBody["response_format"] = format; // Actually JSON Object is only {} not [], so prompt is safer
    }

    QJsonArray messages;
    QJsonObject message;
    message["role"] = "user";
    QString sourceLangStr = (m_sourceLanguage.isEmpty() || m_sourceLanguage == "auto") ? "" : QString(" from %1").arg(m_sourceLanguage);
    
    if (m_isBatchMode) {
        message["content"] = QString("You are translating dialogue and UI text from a video game. You will receive a JSON array of strings. Translate them%1 to %2. Return ONLY a valid JSON array of translated strings in the exact same order. Do not mix writing systems. IMPORTANT: DO NOT TRUNCATE or shorten the translations. Translate EVERYTHING. Do not add markdown blocks like ```json, just return the raw array. Here is the array:\n\n%3").arg(sourceLangStr, m_targetLanguage, sourceText);
    } else {
        message["content"] = QString("You are translating dialogue and UI text from a video game. Translate%1 to %2. Return ONLY the translated text using %2 script/characters. Do not mix writing systems. IMPORTANT: DO NOT TRUNCATE or shorten the translation. Translate EVERYTHING. Do not add explanations.\n\n%3").arg(sourceLangStr, m_targetLanguage, sourceText);
    }
    
    messages.append(message);
    requestBody["messages"] = messages;
}

void LLMTranslationService::buildAnthropicRequest(QNetworkRequest &request, QJsonObject &requestBody, const QString &sourceText)
{
    request.setUrl(QUrl("https://api.anthropic.com/v1/messages"));
    request.setRawHeader("x-api-key", m_apiKey.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");
    requestBody["model"] = m_model;
    requestBody["max_tokens"] = m_isBatchMode ? 4096 : 2048;
    QJsonArray messages;
    QJsonObject message;
    message["role"] = "user";
    QString sourceLangStr = (m_sourceLanguage.isEmpty() || m_sourceLanguage == "auto") ? "" : QString(" from %1").arg(m_sourceLanguage);
    
    if (m_isBatchMode) {
        message["content"] = QString("You are translating dialogue and UI text from a video game. You will receive a JSON array of strings. Translate them%1 to %2. Return ONLY a valid JSON array of translated strings in the exact same order. Do not mix writing systems. IMPORTANT: DO NOT TRUNCATE or shorten the translations. Translate EVERYTHING. Do not add markdown blocks like ```json, just return the raw array. Here is the array:\n\n%3").arg(sourceLangStr, m_targetLanguage, sourceText);
    } else {
        message["content"] = QString("You are translating dialogue and UI text from a video game. Translate%1 to %2. Return ONLY the translated text using %2 script/characters. Do not mix writing systems. IMPORTANT: DO NOT TRUNCATE or shorten the translation. Translate EVERYTHING. Do not add explanations.\n\n%3").arg(sourceLangStr, m_targetLanguage, sourceText);
    }
    
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
    
    if (m_isBatchMode) {
        part["text"] = QString("You are translating dialogue and UI text from a video game. You will receive a JSON array of strings. Translate them%1 to %2. Return ONLY a valid JSON array of translated strings in the exact same order. Do not mix writing systems. IMPORTANT: DO NOT TRUNCATE or shorten the translations. Translate EVERYTHING. Do not add markdown blocks like ```json, just return the raw array. Here is the array:\n\n%3").arg(sourceLangStr, m_targetLanguage, sourceText);
    } else {
        part["text"] = QString("You are translating dialogue and UI text from a video game. Translate%1 to %2. Return ONLY the translated text using %2 script/characters. Do not mix writing systems. IMPORTANT: DO NOT TRUNCATE or shorten the translation. Translate EVERYTHING. Do not add explanations.\n\n%3").arg(sourceLangStr, m_targetLanguage, sourceText);
    }
    
    QJsonArray parts;
    parts.append(part);
    content["parts"] = parts;
    QJsonArray contents;
    contents.append(content);
    requestBody["contents"] = contents;
    
    // Add maxOutputTokens for Google
    QJsonObject generationConfig;
    generationConfig["maxOutputTokens"] = m_isBatchMode ? 4096 : 2048;
    requestBody["generationConfig"] = generationConfig;
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