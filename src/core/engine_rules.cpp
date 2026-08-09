#include "engine_rules.h"
#include <QChar>

static bool isRpgmEngineName(const QString &eng)
{
    return eng.contains("rpgm") || eng.contains("rpgmaker") || eng == "mv" || eng == "mz" || eng == "vx" || eng == "xp";
}

bool EngineRules::shouldSkip(const QString &text, const QString &engineName)
{
    if (text.isEmpty()) return true;

    const QString eng = engineName.trimmed().toLower();

    if (isRpgmEngineName(eng)) return isRpgmNonText(text);
    if (eng.contains("renpy") || eng.contains("unrpyc")) return isRenpyNonText(text);
    if (eng.contains("unity")) return isUnityNonText(text);
    if (eng.contains("wolf")) return isWolfRpgNonText(text);
    if (eng.contains("tyrano") || eng.contains("tyranobuilder")) return isTyranoBuilderNonText(text);
    if (eng.contains("godot")) return isGodotNonText(text);
    if (eng.contains("kirikiri") || eng.contains("tjs") || eng.contains("ks")) return isKirikiriNonText(text);

    return false;
}

bool EngineRules::isRpgmNonText(const QString &text)
{
    // RPG Maker JS Script calls / event code lines that are non-translatable
    static const QRegularExpression rpgmScriptLine("^(this\\.|\\$game|Input\\.|TouchInput\\.|Graphics\\.|AudioManager\\.|SceneManager\\.)");
    if (rpgmScriptLine.match(text).hasMatch()) return true;

    // Common RPG Maker asset references / BGM / SE / BGS / ME / IMG
    static const QRegularExpression rpgmAssetRef("^(bgm|se|bgs|me|img)/", QRegularExpression::CaseInsensitiveOption);
    if (rpgmAssetRef.match(text).hasMatch()) return true;

    // Common RPG Maker Tileset Image Names (e.g. World_A1, Outside_A1, Dungeon_A1, SF_Outside_B)
    static const QRegularExpression rpgmTilesetPattern(R"(^(World|Outside|Inside|Dungeon|SF_Outside|SF_Inside|SF_Outside_A\d|SF_Inside_A\d)_[A-E][1-5]?$)", QRegularExpression::CaseInsensitiveOption);
    if (rpgmTilesetPattern.match(text).hasMatch()) return true;

    return false;
}

bool EngineRules::isRenpyNonText(const QString &text)
{
    // Ren'Py Python inline statements, image declarations, play sound/music
    static const QRegularExpression renpyCodeLine("^(define|default|label|init|python|image|play|stop|scene|show|hide|jump|call)\\s+");
    if (renpyCodeLine.match(text).hasMatch()) return true;

    static const QRegularExpression renpyVarAccess("^(renpy\\.|config\\.|persistent\\.|store\\.)");
    if (renpyVarAccess.match(text).hasMatch()) return true;

    return false;
}

bool EngineRules::isUnityNonText(const QString &text)
{
    // Unity asset GUIDs (32-char hex string)
    static const QRegularExpression unityAssetGuid("^[0-9a-fA-F]{32}$");
    if (unityAssetGuid.match(text).hasMatch()) return true;

    // Unity Component property paths
    static const QRegularExpression unityPropPath("^(m_|m_Local|m_Name|m_Script|m_GameObject)");
    if (unityPropPath.match(text).hasMatch()) return true;

    return false;
}

bool EngineRules::isWolfRpgNonText(const QString &text)
{
    // Wolf RPG Database & Variable prefixes
    static const QRegularExpression wolfVar("^(cDB:|uDB:|sVar:|vVar:)");
    if (wolfVar.match(text).hasMatch()) return true;

    return false;
}

bool EngineRules::isTyranoBuilderNonText(const QString &text)
{
    // TyranoBuilder / TyranoScript tags (e.g. [tb_start_text], [cm], [playse], [bg])
    static const QRegularExpression tyranoTag("^\\[(tb_|[a-z]{2,8}\\b)");
    if (tyranoTag.match(text).hasMatch()) return true;

    return false;
}

bool EngineRules::isGodotNonText(const QString &text)
{
    // Godot Engine GDScript keywords, resource paths (res://), PCK headers
    static const QRegularExpression godotCode("^(res://|var\\s+|func\\s+|extends\\s+|const\\s+|signal\\s+|enum\\s+)");
    if (godotCode.match(text).hasMatch()) return true;

    return false;
}

bool EngineRules::isKirikiriNonText(const QString &text)
{
    // KiriKiri / KAG / TJS script tags and commands (@r, @pg, @locate, @image)
    static const QRegularExpression kirikiriTag("^@[a-z]+|^\\*[a-zA-Z0-9_]+");
    if (kirikiriTag.match(text).hasMatch()) return true;

    return false;
}

bool EngineRules::isGlobalNonText(const QString &text)
{
    if (text.isEmpty()) return true;

    // Quick non-ASCII check (Kanji, Hiragana, Katakana, Thai, Cyrillic, Hangul)
    for (const QChar &c : text) {
        ushort u = c.unicode();
        if ((u >= 0x3000 && u <= 0x9FFF) || (u >= 0x0E00 && u <= 0x0E7F) || 
            (u >= 0x0400 && u <= 0x04FF) || (u >= 0xAC00 && u <= 0xD7AF)) {
            return false;
        }
    }

    if (isNumericOrSymbol(text)) return true;
    if (isFilePathOrMedia(text)) return true;
    if (isVariableLike(text)) return true;
    if (isCamelCase(text)) return true;
    if (isSnakeCase(text)) return true;
    if (isTagOrMarkup(text)) return true;
    if (isTechnicalOrHex(text)) return true;
    if (isRepeatedSymbol(text)) return true;

    return false;
}

bool EngineRules::isNumericOrSymbol(const QString &text)
{
    static QRegularExpression regex("^[^\\p{L}]+$"); 
    return regex.match(text).hasMatch();
}

bool EngineRules::isFilePathOrMedia(const QString &text)
{
    if (text.contains(" ")) return false; // Natural dialogue with spaces is not a file path
    static const QRegularExpression mediaExt(R"(\.(png|jpg|jpeg|gif|bmp|webp|ogg|mp3|wav|m4a|flac|json|mat|prefab|asset|dll|so|exe)$)", QRegularExpression::CaseInsensitiveOption);
    if (mediaExt.match(text).hasMatch()) return true;
    if (text.startsWith("img/") || text.startsWith("audio/") || text.startsWith("bgm/") || text.startsWith("se/") || text.startsWith("bgs/") || text.startsWith("data/")) return true;
    return false;
}

bool EngineRules::isVariableLike(const QString &text)
{
    static QRegularExpression regex("^[A-Za-z]+_[0-9]+$");
    return regex.match(text).hasMatch();
}

bool EngineRules::isCamelCase(const QString &text)
{
    if (text.contains(" ")) return false;
    if (text.length() < 3) return false;
    // Only match lowerCamelCase method/variable identifiers like getScriptPath, onButtonClick
    static QRegularExpression regex("^[a-z]+[A-Z0-9][a-zA-Z0-9]*$");
    return regex.match(text).hasMatch();
}

bool EngineRules::isSnakeCase(const QString &text)
{
    if (text.contains(" ")) return false;
    if (!text.contains("_")) return false;
    
    // Only match system variables like sys_var, img_01, snd_click
    static QRegularExpression sysVar(R"(^(sys|img|snd|bgm|se|bgs|flag|var|tmp|str|num|id|key)_[a-zA-Z0-9_]+$)", QRegularExpression::CaseInsensitiveOption);
    return sysVar.match(text).hasMatch();
}

bool EngineRules::isTagOrMarkup(const QString &text)
{
    // Only match pure empty system HTML/XML tags without inner text, like "<br/>", "</p>", "<hr>"
    static QRegularExpression systemTag(R"(^<[a-zA-Z0-9_\-\s="/]+>$)", QRegularExpression::CaseInsensitiveOption);
    return systemTag.match(text).hasMatch();
}

bool EngineRules::isTechnicalOrHex(const QString &text)
{
    static QRegularExpression regex("^(0x[0-9A-Fa-f]+|[A-Za-z0-9]{1,4})$");
    if (regex.match(text).hasMatch()) return true;
    
    int symbolCount = 0;
    for (const QChar &c : text) {
        if (!c.isLetterOrNumber() && !c.isSpace()) symbolCount++;
    }
    if (text.length() > 0 && (double)symbolCount / text.length() > 0.5) return true;

    return false;
}

bool EngineRules::isRepeatedSymbol(const QString &text)
{
    if (text.length() < 3) return false;
    QChar first = text.at(0);
    if (first.isLetterOrNumber()) return false;
    
    for (const QChar &c : text) {
        if (c != first) return false;
    }
    return true;
}
