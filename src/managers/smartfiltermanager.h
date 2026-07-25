#ifndef SMARTFILTERMANAGER_H
#define SMARTFILTERMANAGER_H

#include <QObject>
#include <QStringList>
#include <QRegularExpression>
#include <QSettings>
#include "../core/engine_rules.h"

/**
 * @class SmartFilterManager
 * @brief High-performance Manager interfacing with EngineRules & learned user patterns.
 */
class SmartFilterManager : public QObject
{
    Q_OBJECT
public:
    explicit SmartFilterManager(QObject *parent = nullptr);
    ~SmartFilterManager() override = default;

    // Adds a text pattern to the ignore list
    void learn(const QString &text);

    // Removes a text pattern from the ignore list
    void unlearn(const QString &text);

    // Sets the current engine context (e.g. "rpgm", "renpy", "unity", "wolf")
    void setEngine(const QString &engineName);
    QString currentEngine() const { return m_currentEngine; }

    // Export all learned rules to a JSON file
    bool exportRules(const QString &filePath);

    // Import rules from a JSON file and merge with existing ones
    bool importRules(const QString &filePath);

    // Checks if the text should be skipped based on engine rules, learned patterns, or heuristics
    bool shouldSkip(const QString &text) const;

    // Checks a batch of texts and returns a list of booleans (true = skip)
    QList<bool> shouldSkipBatch(const QStringList &texts) const;

    // Returns the list of ignored patterns
    QStringList ignoredPatterns() const;
    
    // Engine Filter Configuration
    void setEngineFilterEnabled(bool enabled);
    bool isEngineFilterEnabled() const;

public slots:
    void savePatterns();
    void loadPatterns();

private:
    QStringList m_ignoredPatterns;
    QList<QRegularExpression> m_compiledPatterns;
    QString m_currentEngine = "Global";
    bool m_engineFilterEnabled = true;
};

#endif // SMARTFILTERMANAGER_H
