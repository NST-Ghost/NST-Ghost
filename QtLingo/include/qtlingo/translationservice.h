#ifndef QTLINGO_TRANSLATIONSERVICE_H
#define QTLINGO_TRANSLATIONSERVICE_H

#include "QtLingo_global.h"
#include "translationsettings.h"
#include <QString>
#include <QStringList>
#include <QObject>
#include <QVariantMap>

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
    virtual QString serviceName() const = 0; // Note: serviceName doesn't override but make sure it compiles
    virtual void translate(const QString &sourceText) = 0;

    // Common settings (used by most services)
    virtual void setApiKey(const QString &apiKey) { Q_UNUSED(apiKey); }
    virtual void setTargetLanguage(const QString &language) { Q_UNUSED(language); }
    virtual void setSourceLanguage(const QString &language) { Q_UNUSED(language); }

    // Generic configuration — each service extracts what it needs from the settings
    virtual void configure(const TranslationSettings &settings) { Q_UNUSED(settings); }

    // Batch Translation
    virtual bool supportsBatchTranslation() const { return false; }
    virtual void batchTranslate(const QStringList &sourceTexts) { Q_UNUSED(sourceTexts); }

signals:
    void translationFinished(const TranslationResult &result);
    void batchTranslationFinished(const QList<TranslationResult> &results);
    void errorOccurred(const QString &message);
    void logMessage(const QString &message);
};

} // namespace qtlingo

#endif // QTLINGO_TRANSLATIONSERVICE_H