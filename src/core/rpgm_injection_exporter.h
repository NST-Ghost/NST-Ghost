#ifndef RPGM_INJECTION_EXPORTER_H
#define RPGM_INJECTION_EXPORTER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QJsonArray>

/**
 * @brief Exports NST translation data as a non-destructive JS injection layer
 *        for RPG Maker MV/MZ games.
 *
 * Instead of modifying original game JSON files, this exporter:
 *   1. Creates a nst_translations/ folder inside the game root
 *   2. Writes translation .json files (one per source data file):
 *        { "source_text": "translated_text", ... }
 *   3. Writes a config.json settings file that drives the runtime hooks:
 *        { "__customHooks__": [...], "__textFields__": [...],
 *          "__textCommands__": {...}, "__controlCharPatterns__": [...],
 *          "__ignorePatterns__": [...], "__fontConfig__": {...},
 *          "__sourceLocale__": "ja" }
 *   4. Copies the NST_TranslationLayer.js plugin to js/plugins/
 *
 * The JS plugin then hooks into the RPG Maker runtime and translates
 * text on-the-fly using data from these external JSON files.
 */
class RpgmInjectionExporter : public QObject
{
    Q_OBJECT
public:
    explicit RpgmInjectionExporter(QObject *parent = nullptr);

    /**
     * @brief Deploy translation data as an injection layer.
     * @param gamePath      Root directory of the RPG Maker game
     * @param data          Translation data (file path → entries array)
     * @param onlyTranslated If true, skip entries without translations
     * @param languageName  Language name for config (e.g., "Thai")
     * @return true on success
     */
    bool deploy(const QString &gamePath,
                const QMap<QString, QJsonArray> &data,
                bool onlyTranslated = true,
                const QString &languageName = QStringLiteral("Thai"));

signals:
    void errorOccurred(const QString &message);
    void progressUpdated(int percentage, const QString &message);

private:
    bool createTranslationFolder(const QString &gamePath);
    bool writeConfigFile(const QString &translationDir, const QString &languageName);
    bool writeTranslationFile(const QString &translationDir,
                              const QString &baseName,
                              const QJsonArray &entries,
                              bool onlyTranslated);
    bool copyPluginFile(const QString &gamePath);
    void patchPluginsJs(const QString &gamePath);

    QString fileBaseName(const QString &filePath) const;
    static QString pluginSourcePath();

    // Build the default __customHooks__ array for config.json. These mirror
    // the legacy hardcoded prototype patching and can be edited by users.
    QJsonArray defaultHooks() const;
};

#endif // RPGM_INJECTION_EXPORTER_H
