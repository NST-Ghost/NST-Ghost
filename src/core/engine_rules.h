#ifndef ENGINE_RULES_H
#define ENGINE_RULES_H

#include <QString>
#include <QRegularExpression>

/**
 * @class EngineRules
 * @brief High-performance, modular rule engine for game text filtering.
 * 
 * Houses all deterministic filtering rules separated by game engine 
 * (RPG Maker, Ren'Py, Unity, Wolf RPG, etc.) plus shared global heuristics.
 */
class EngineRules
{
public:
    // Main entry point for engine-specific text filtering
    static bool shouldSkip(const QString &text, const QString &engineName);

    // --- Engine-Specific Inspectors ---
    static bool isRpgmNonText(const QString &text);
    static bool isRenpyNonText(const QString &text);
    static bool isUnityNonText(const QString &text);
    static bool isWolfRpgNonText(const QString &text);
    static bool isTyranoBuilderNonText(const QString &text);
    static bool isGodotNonText(const QString &text);
    static bool isKirikiriNonText(const QString &text);

    // --- Shared Global Heuristics ---
    static bool isGlobalNonText(const QString &text);
    static bool isNumericOrSymbol(const QString &text);
    static bool isFilePathOrMedia(const QString &text);
    static bool isVariableLike(const QString &text);
    static bool isCamelCase(const QString &text);
    static bool isSnakeCase(const QString &text);
    static bool isTagOrMarkup(const QString &text);
    static bool isTechnicalOrHex(const QString &text);
    static bool isRepeatedSymbol(const QString &text);
};

#endif // ENGINE_RULES_H
