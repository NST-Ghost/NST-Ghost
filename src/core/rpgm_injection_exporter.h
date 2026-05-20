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
 *   2. Writes translation .txt files (one per source data file)
 *   3. Writes a config.txt settings file
 *   4. Copies the NST_TranslationLayer.js plugin to js/plugins/
 *
 * The JS plugin then hooks into the RPG Maker runtime and translates
 * text on-the-fly using data from these external files.
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
};

#endif // RPGM_INJECTION_EXPORTER_H
