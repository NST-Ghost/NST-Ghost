#ifndef RPGM_CONTROL_MASKER_H
#define RPGM_CONTROL_MASKER_H

#include <QString>
#include <QMap>
#include <QRegularExpression>

/**
 * @class RpgmControlMasker
 * @brief Pre-translation placeholder masking & restoration ONLY for RPG Maker games.
 * 
 * Masks RPG Maker escape control codes (\V[n], \N[n], \C[n], \I[n], \FS[n], \MSGCORE[...], etc.)
 * into safe, immutable placeholders (__NST_TAG_0__, __NST_TAG_1__, ...) before sending text to AI/Google Translation,
 * and restores original control codes with 100% precision afterwards.
 */
class RpgmControlMasker
{
public:
    struct MaskResult {
        QString maskedText;
        QMap<QString, QString> tagMap;
        bool hasMaskedTags = false;
    };

    /**
     * @brief Masks RPG Maker control codes in source text before translation.
     */
    static MaskResult mask(const QString &sourceText);

    /**
     * @brief Restores original RPG Maker control codes from tagMap in translated text.
     */
    static QString unmask(const QString &translatedText, const QMap<QString, QString> &tagMap);

    /**
     * @brief Checks if an engine name string corresponds to RPG Maker.
     */
    static bool isRpgmEngine(const QString &engineName);
};

#endif // RPGM_CONTROL_MASKER_H
