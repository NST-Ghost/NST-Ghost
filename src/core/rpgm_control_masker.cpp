#include "rpgm_control_masker.h"
#include <QRegularExpressionMatchIterator>
#include <QDebug>

RpgmControlMasker::MaskResult RpgmControlMasker::mask(const QString &sourceText)
{
    MaskResult result;
    if (sourceText.isEmpty()) {
        result.maskedText = sourceText;
        return result;
    }

    // Combine all RPG Maker escape code patterns (matching TS specification)
    // Matches \V[n], \N[n], \P[n], \I[n], \C[n], \G, \$, \., \|, \!, \^, \{, \}, \\, \FS[n], \PX[n], \PY[n], \OC[n], \TC[n], \MSGCORE[...]
    static const QRegularExpression rpgmControlRegex(
        QStringLiteral(
            "("
            "\\\\\\\\[VNP]\\[\\d+\\]"                                 // \\V[n], \\N[n], \\P[n]
            "|\\\\\\\\I\\[\\d+\\]"                                   // \\I[n]
            "|\\\\\\\\C\\[\\d+\\]"                                   // \\C[n]
            "|\\\\\\\\G"                                             // \\G
            "|\\\\\\\\[{}]"                                          // \\{ and \\}
            "|\\\\\\\\\\$"                                           // \\$
            "|\\\\\\\\[.|]"                                          // \\. and \\|
            "|\\\\\\\\!"                                             // \\!
            "|\\\\\\\\[><]"                                          // \\> and \\<
            "|\\\\\\\\\\^"                                           // \\^
            "|\\\\\\\\\\\\"                                          // \\\\
            "|\\\\\\\\FS\\[\\d+\\]"                                  // \\FS[n]
            "|\\\\\\\\P[XY]\\[-?\\d+\\]"                             // \\PX[n], \\PY[n]
            "|\\\\\\\\[OT]C\\[\\d+\\]"                               // \\OC[n], \\TC[n]
            "|\\\\\\\\(?:MSGCORE|MSGSND)\\[[^\\]]*\\]"              // \\MSGCORE[...], \\MSGSND[...]
            "|\\\\[VNP]\\[\\d+\\]"                                   // \V[n], \N[n], \P[n]
            "|\\\\[IC]\\[\\d+\\]"                                    // \I[n], \C[n]
            "|\\\\[G!^\\$]"                                          // \G, \!, \^, \$
            "|\\\\[{}]"                                              // \{, \}
            "|\\\\[.|]"                                              // \., \|
            "|\\\\FS\\[\\d+\\]"                                      // \FS[n]
            "|\\\\P[XY]\\[-?\\d+\\]"                                 // \PX[n], \PY[n]
            "|\\\\[OT]C\\[\\d+\\]"                                   // \OC[n], \TC[n]
            "|\\\\(?:MSGCORE|MSGSND)\\[[^\\]]*\\]"                  // \MSGCORE[...], \MSGSND[...]
            ")"
        ),
        QRegularExpression::CaseInsensitiveOption
    );

    QString text = sourceText;
    QRegularExpressionMatchIterator it = rpgmControlRegex.globalMatch(sourceText);
    int tagIndex = 0;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString controlCode = match.captured(0);
        
        // Skip if this exact control code is already mapped to avoid duplicate tag creation
        bool alreadyMapped = false;
        for (auto mapIt = result.tagMap.constBegin(); mapIt != result.tagMap.constEnd(); ++mapIt) {
            if (mapIt.value() == controlCode) {
                alreadyMapped = true;
                break;
            }
        }

        if (!alreadyMapped) {
            QString tag = QString("__NST_TAG_%1__").arg(tagIndex++);
            result.tagMap[tag] = controlCode;
            result.hasMaskedTags = true;
        }
    }

    if (result.hasMaskedTags) {
        for (auto mapIt = result.tagMap.constBegin(); mapIt != result.tagMap.constEnd(); ++mapIt) {
            text.replace(mapIt.value(), mapIt.key());
        }
    }

    result.maskedText = text;
    return result;
}

QString RpgmControlMasker::unmask(const QString &translatedText, const QMap<QString, QString> &tagMap)
{
    if (tagMap.isEmpty() || translatedText.isEmpty()) {
        return translatedText;
    }

    QString text = translatedText;

    for (auto it = tagMap.constBegin(); it != tagMap.constEnd(); ++it) {
        const QString &tag = it.key();
        const QString &originalCode = it.value();

        // 1. Direct replacement
        text.replace(tag, originalCode);

        // 2. Flexible replacement for space-corrupted or case-mutated tags from AI/Google Translate
        QString tagNumber = tag;
        tagNumber.remove("__NST_TAG_").remove("__");

        QString flexPattern = QString(QStringLiteral("__\\s*NST\\s*_\\s*TAG\\s*_\\s*%1\\s*__")).arg(tagNumber);
        QRegularExpression flexRegex(flexPattern, QRegularExpression::CaseInsensitiveOption);
        text.replace(flexRegex, originalCode);
    }

    return text;
}

bool RpgmControlMasker::isRpgmEngine(const QString &engineName)
{
    QString eng = engineName.trimmed().toLower();
    return eng.contains("rpgm") || eng.contains("rpgmaker") || eng == "mv" || eng == "mz" || eng == "vx" || eng == "xp";
}
