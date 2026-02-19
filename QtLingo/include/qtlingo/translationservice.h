#ifndef QTLINGO_TRANSLATIONSERVICE_H
#define QTLINGO_TRANSLATIONSERVICE_H

#include "QtLingo_global.h"
#include <QString>
#include <QStringList>
#include <QObject>

namespace qtlingo {

struct TranslationResult {
    QString sourceText;
    QString translatedText;
    // Potentially add more fields like confidence, error message, etc.
};

class QTLINGO_EXPORT ITranslationService : public QObject {
    Q_OBJECT
public:
    explicit ITranslationService(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ITranslationService() = default;
    virtual QString serviceName() const = 0;
    virtual void translate(const QString &sourceText) = 0;

    virtual void setApiKey(const QString &apiKey) { Q_UNUSED(apiKey); }
    virtual void setTargetLanguage(const QString &language) { Q_UNUSED(language); }
    virtual void setSourceLanguage(const QString &language) { Q_UNUSED(language); }

    // Google Translate specific
    virtual void setGoogleTranslateMode(bool isApi) { Q_UNUSED(isApi); }

    // LLM specific
    virtual void setLlmProvider(const QString &provider) { Q_UNUSED(provider); }
    virtual void setLlmModel(const QString &model) { Q_UNUSED(model); }
    virtual void setLlmEndpoint(const QString &endpoint) { Q_UNUSED(endpoint); }

    // Batch Translation
    virtual bool supportsBatchTranslation() const { return false; }
    virtual void batchTranslate(const QStringList &sourceTexts) { Q_UNUSED(sourceTexts); }

signals:
    void translationFinished(const TranslationResult &result);
    void batchTranslationFinished(const QList<TranslationResult> &results);
    void errorOccurred(const QString &message);
};

} // namespace qtlingo

#endif // QTLINGO_TRANSLATIONSERVICE_H