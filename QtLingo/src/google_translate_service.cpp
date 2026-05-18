#include "google_translate_service.h"
#include <QDebug>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

namespace qtlingo {

GoogleTranslateService::GoogleTranslateService(QObject *parent)
    : ITranslationService(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &GoogleTranslateService::onNetworkReply);
}

void GoogleTranslateService::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void GoogleTranslateService::setTargetLanguage(const QString &language)
{
    m_targetLanguage = language;
}

void GoogleTranslateService::setSourceLanguage(const QString &language)
{
    m_sourceLanguage = language;
}

void GoogleTranslateService::setGoogleTranslateMode(bool isApi)
{
    m_isApi = isApi;
}

void GoogleTranslateService::configure(const QVariantMap &settings)
{
    setTargetLanguage(settings.value("targetLanguage").toString());
    setSourceLanguage(settings.value("sourceLanguage", "auto").toString());
    m_isApi = settings.value("googleApi").toBool();
    if (m_isApi) {
        setApiKey(settings.value("googleApiKey").toString());
    }
}

void GoogleTranslateService::translate(const QString &sourceText)
{
    // Preprocess: Mask RPGM codes
    QMap<QString, QString> tagMap;
    QString maskedText = preprocessText(sourceText, tagMap);

    // Calls to old helpers removed as logic is now inlined below.
    
    // Store request data for the *most recent* request
    // Note: helpers perform async calls. We unfortunately can't capture the reply easily here 
    // without refactoring helpers to return it.
    // Given the critical error state, I will manually inline the logic to capture the reply,
    // ensuring the RequestData is stored properly.
    
    QNetworkReply *reply = nullptr;
    if (m_isApi) {
        // ... (Inline translateWithApi logic)
        if (m_apiKey.isEmpty()) {
            emit errorOccurred("API key is not set for Google Translate.");
            return;
        }
        QUrl url("https://translation.googleapis.com/language/translate/v2");
        QUrlQuery query;
        query.addQueryItem("q", maskedText);
        query.addQueryItem("target", m_targetLanguage);
        query.addQueryItem("key", m_apiKey);
        query.addQueryItem("format", "text");
        if (m_sourceLanguage != "auto") query.addQueryItem("source", m_sourceLanguage);
        url.setQuery(query);
        QNetworkRequest request(url);
        request.setTransferTimeout(60000);
        request.setAttribute(QNetworkRequest::User, QVariant(true));
        reply = m_networkManager->get(request);
    } else {
        // ... (Inline translateWithFreeApi logic)
        QString urlString = QString("https://translate.googleapis.com/translate_a/single?client=gtx&sl=%1&tl=%2&dt=t&q=%3")
                            .arg(m_sourceLanguage)
                            .arg(m_targetLanguage)
                            .arg(QString(QUrl::toPercentEncoding(maskedText)));
        QUrl url(urlString);
        QNetworkRequest request(url);
        request.setTransferTimeout(60000);
        request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
        request.setAttribute(QNetworkRequest::User, QVariant(false));
        reply = m_networkManager->get(request);
    }

    if (reply) {
        RequestData reqData;
        reqData.isBatch = false;
        reqData.isApi = m_isApi;
        reqData.sourceText = sourceText;
        reqData.tagMap = tagMap;
        m_activeRequests.insert(reply, reqData);
    }
}

void GoogleTranslateService::batchTranslate(const QStringList &sourceTexts)
{
    if (m_isApi) {
        // API mode: use single JSON batch request (reliable mapping)
        RequestData reqData;
        reqData.isBatch = true;
        reqData.isApi = true;
        reqData.batchSourceTexts = sourceTexts;

        QStringList maskedTexts;
        for (const QString &text : sourceTexts) {
            QMap<QString, QString> map;
            maskedTexts.append(preprocessText(text, map));
            reqData.batchTagMaps.append(map);
        }

        if (m_apiKey.isEmpty()) {
            emit errorOccurred("API key is not set for Google Translate.");
            return;
        }

        QUrl url("https://translation.googleapis.com/language/translate/v2");
        QUrlQuery query;
        for (const QString &text : maskedTexts) {
            query.addQueryItem("q", text);
        }
        query.addQueryItem("target", m_targetLanguage);
        query.addQueryItem("key", m_apiKey);
        query.addQueryItem("format", "text");
        if (m_sourceLanguage != "auto") {
            query.addQueryItem("source", m_sourceLanguage);
        }

        url.setQuery(query);
        QNetworkRequest request(url);
        request.setTransferTimeout(120000);
        request.setAttribute(QNetworkRequest::User, QVariant(true));
        QNetworkReply *reply = m_networkManager->get(request);
        if (reply) {
            m_activeRequests.insert(reply, reqData);
        }
    } else {
        // Free mode: use concurrent pool of individual requests
        // Cancel any inflight requests from previous batch
        QList<QNetworkReply*> staleReplies;
        for (auto it = m_activeRequests.constBegin(); it != m_activeRequests.constEnd(); ++it) {
            if (it.value().batchIndex >= 0) {
                staleReplies.append(it.key());
            }
        }
        for (QNetworkReply *r : staleReplies) {
            m_activeRequests.remove(r);
            r->abort();
            r->deleteLater();
        }

        m_batchState = BatchState();
        m_batchState.totalCount = sourceTexts.size();
        m_batchState.sourceTexts = sourceTexts;
        m_batchState.results.resize(m_batchState.totalCount);
        m_batchState.nextIndex = 0;
        m_batchState.completedCount = 0;

        // Preprocess all texts and store tag maps
        for (const QString &text : sourceTexts) {
            QMap<QString, QString> map;
            preprocessText(text, map);
            m_batchState.tagMaps.append(map);
        }

        dispatchNextFreeRequests();
    }
}

void GoogleTranslateService::dispatchNextFreeRequests()
{
    int inflight = m_batchState.nextIndex - m_batchState.completedCount;
    while (m_batchState.nextIndex < m_batchState.totalCount
           && inflight < BatchState::POOL_SIZE) {
        int idx = m_batchState.nextIndex++;
        inflight++;

        // Preprocess this text
        QMap<QString, QString> tagMap;
        QString maskedText = preprocessText(m_batchState.sourceTexts.at(idx), tagMap);
        m_batchState.tagMaps[idx] = tagMap;

        QString urlString = QString("https://translate.googleapis.com/translate_a/single?client=gtx&sl=%1&tl=%2&dt=t&q=%3")
                            .arg(m_sourceLanguage)
                            .arg(m_targetLanguage)
                            .arg(QString(QUrl::toPercentEncoding(maskedText)));

        QUrl url(urlString);
        QNetworkRequest request(url);
        request.setTransferTimeout(60000);
        request.setHeader(QNetworkRequest::UserAgentHeader,
                         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

        QNetworkReply *reply = m_networkManager->get(request);
        if (reply) {
            RequestData reqData;
            reqData.isApi = false;
            reqData.isBatch = false;  // Individual request
            reqData.batchIndex = idx; // Track position in batch
            reqData.sourceText = m_batchState.sourceTexts.at(idx);
            reqData.tagMap = tagMap;
            m_activeRequests.insert(reply, reqData);
        }
    }
}

// Refactored helpers to RETURN reply or take reference?
// Since I can't easily change header, I'll essentially reimplement 'translate' to use the logic inline or modify these.
// Waiting on header change capability. I DID change header to remove members.
// So I should comment these out or unused them, and put logic in 'translate'.
// Or better: update these to return QNetworkReply* and change header.
// I'll update 'translate' to do the work directly and ignore these helpers for now to avoid signature mismatch errors if I don't update header signatures.
// Wait, I didn't update helpers signatures in header.
// So I will implement 'translate' fully and LEAVE these empty or unused.
// Actually, safer to keep logic in 'translate'.


QString GoogleTranslateService::extractTranslationFromHtml(const QString &html)
{
    // Try to parse JSON response from free API
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(html.toUtf8(), &parseError);

    if (parseError.error == QJsonParseError::NoError && jsonDoc.isArray()) {
        QJsonArray mainArray = jsonDoc.array();
        if (mainArray.size() > 0 && mainArray[0].isArray()) {
            QJsonArray translationArray = mainArray[0].toArray();
            QString result;

            // Concatenate all translation parts
            for (const QJsonValue &value : translationArray) {
                if (value.isArray()) {
                    QJsonArray partArray = value.toArray();
                    if (partArray.size() > 0) {
                        result += partArray[0].toString();
                    }
                }
            }

            return result;
        }
    }

    return "[Error: Could not parse translation]";
}

void GoogleTranslateService::onNetworkReply(QNetworkReply *reply)
{
    // Retrieve request state
    if (!m_activeRequests.contains(reply)) {
        // Unknown reply or already processed?
        reply->deleteLater();
        return;
    }
    RequestData reqData = m_activeRequests.take(reply);

    TranslationResult result;
    result.sourceText = reqData.sourceText;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        // bool isApiRequest = ... (We trust reqData.isApi)
        bool isApiRequest = reqData.isApi;
        bool isBatchRequest = reqData.isBatch;

        if (isApiRequest) {
            // Parse official API response
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            QJsonObject jsonObj = jsonDoc.object();
            QJsonObject dataObj = jsonObj["data"].toObject();
            QJsonArray translations = dataObj["translations"].toArray();

            if (isBatchRequest) {
                 QList<TranslationResult> results;
                 if (translations.size() == reqData.batchSourceTexts.size()) {
                     for (int i = 0; i < translations.size(); ++i) {
                         TranslationResult r;
                         r.sourceText = reqData.batchSourceTexts.at(i);
                         QString rawTranslated = translations[i].toObject()["translatedText"].toString();
                         // Postprocess Batch
                         if (i < reqData.batchTagMaps.size()) {
                             r.translatedText = postprocessText(rawTranslated, reqData.batchTagMaps[i]);
                         } else {
                             r.translatedText = rawTranslated;
                         }
                         results.append(r);
                     }
                     emit batchTranslationFinished(results);
                 } else {
                     emit errorOccurred("Batch translation count mismatch");
                 }
            } else {
                if (!translations.isEmpty()) {
                    QString rawTranslated = translations[0].toObject()["translatedText"].toString();
                    result.translatedText = postprocessText(rawTranslated, reqData.tagMap);
                    emit translationFinished(result);
                } else {
                    emit errorOccurred("No translation found in API response");
                }
            }
        } else {
            // Parse free API response
            QString fullTranslation = extractTranslationFromHtml(QString::fromUtf8(responseData));

            if (reqData.batchIndex >= 0) {
                // Concurrent pool result — store at correct index
                // Safety: check bounds in case batch was reset by a new translate() call
                if (reqData.batchIndex >= m_batchState.results.size()) {
                    // Stale reply from a previous batch — ignore
                    reply->deleteLater();
                    return;
                }
                TranslationResult r;
                r.sourceText = reqData.sourceText;
                r.translatedText = postprocessText(fullTranslation, reqData.tagMap);
                m_batchState.results[reqData.batchIndex] = r;
                m_batchState.completedCount++;

                if (m_batchState.completedCount == m_batchState.totalCount) {
                    // All done — emit full batch
                    emit batchTranslationFinished(m_batchState.results);
                } else {
                    // Dispatch more if available
                    dispatchNextFreeRequests();
                }
            } else {
                // Single free request
                result.translatedText = postprocessText(fullTranslation, reqData.tagMap);
                emit translationFinished(result);
            }
        }

    } else {
        qDebug() << "Network Error:" << reply->errorString();

        if (reqData.batchIndex >= 0) {
            // Safety: check bounds in case batch was reset
            if (reqData.batchIndex >= m_batchState.results.size()) {
                reply->deleteLater();
                return;
            }
            // Concurrent pool error — mark as completed with empty result
            TranslationResult r;
            r.sourceText = reqData.sourceText;
            r.translatedText = "";
            m_batchState.results[reqData.batchIndex] = r;
            m_batchState.completedCount++;

            qDebug() << "Batch item" << reqData.batchIndex << "failed:" << reply->errorString();

            if (m_batchState.completedCount == m_batchState.totalCount) {
                emit batchTranslationFinished(m_batchState.results);
            } else {
                dispatchNextFreeRequests();
            }
        } else {
            emit errorOccurred(reply->errorString());
        }
    }

    reply->deleteLater();
}

QString GoogleTranslateService::preprocessText(const QString &text, QMap<QString, QString> &map)
{
    QString result;
    map.clear();
    // Regex for RPGM Codes: \V[n], \I[n], \C[n], \., \|, \!, \$, etc.
    // Matches backslash followed by:
    // 1. Alphanumeric Word + Optional [Digits]
    // 2. OR Non-Alphanumeric character (symbol)
    QRegularExpression re("\\\\([A-Za-z]+(?:\\[\\d+\\])?|[^A-Za-z0-9\\s])");

    int lastPos = 0;
    int tagCounter = 0;
    QRegularExpressionMatchIterator i = re.globalMatch(text);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        result.append(text.mid(lastPos, match.capturedStart() - lastPos));

        QString original = match.captured(0);
        // Use a format that translation likely won't touch.
        // Spaces help isolate it so Google ignores it.
        QString placeholder = QString(" __TAG_%1__ ").arg(tagCounter++);
        map.insert(placeholder.trimmed(), original);

        result.append(placeholder);
        lastPos = match.capturedEnd();
    }
    result.append(text.mid(lastPos));

    return result;
}

QString GoogleTranslateService::postprocessText(const QString &text, const QMap<QString, QString> &map)
{
    QString result = text;
    QMapIterator<QString, QString> i(map);
    while (i.hasNext()) {
        i.next();
        QString tag = i.key(); // "__TAG_0__"
        QString original = i.value();

        // Regex to find tag with potential extra spaces/formatting
        QString pattern = QString("\\s*%1\\s*").arg(QRegularExpression::escape(tag));
        result.replace(QRegularExpression(pattern), original);
    }
    return result;
}

} // namespace qtlingo
