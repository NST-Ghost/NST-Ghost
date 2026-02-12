#include "smartfiltermanager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSet>


#ifdef HAS_PYTHON
#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#pragma pop_macro("slots")

namespace py = pybind11;

struct SmartFilterManager::SmartFilterManagerPrivate {
    py::object m_pyFilter;
};
#endif

SmartFilterManager::SmartFilterManager(QObject *parent)
    : QObject(parent)
{
#ifdef HAS_PYTHON
    // Acquire GIL BEFORE creating Private struct, because py::object's
    // default constructor calls inc_ref() on Py_None which requires the GIL.
    py::gil_scoped_acquire acquire;
    d = std::make_unique<SmartFilterManagerPrivate>();

    // Initialize Python connection
    try {
        py::module_ sys = py::module_::import("sys");
        sys.attr("path").attr("append")(".");
        sys.attr("path").attr("append")("pylib");
        
        py::module_ ai_mod = py::module_::import("pylib.ai_smart_filter");
        
        d->m_pyFilter = ai_mod.attr("AISmartFilter")();
        
        // Load initial state
        d->m_pyFilter.attr("load_state")("ai_filter_config.json");
        qDebug() << "AI Smart Filter initialized successfully.";
    } catch (const std::exception &e) {
        qDebug() << "Failed to initialize AI Smart Filter:" << e.what();
        d->m_pyFilter = py::none();
    }
#endif

    loadPatterns();
}

SmartFilterManager::~SmartFilterManager() = default;

void SmartFilterManager::learn(const QString &text)
{
    if (text.isEmpty()) return;

    // Simple learning: Add exact match or simple wildcard if it looks like a pattern
    // For now, we'll just add it as an exact match or a simple regex if it contains numbers
    
    QString pattern = QRegularExpression::escape(text);
    
    // If it ends with numbers, generalize it (e.g., "Var_001" -> "Var_\d+")
    static QRegularExpression numberSuffix("_?\\d+$");
    if (text.contains(numberSuffix)) {
        pattern = text;
        pattern.replace(numberSuffix, "_?\\\\d+");
        // Un-escape the rest but keep the regex part? No, safer to escape everything then replace.
        pattern = QRegularExpression::escape(text);
        pattern.replace(QRegularExpression("_?\\d+$"), "_?\\\\d+");
    }

    if (!m_ignoredPatterns.contains(pattern)) {
        m_ignoredPatterns.append(pattern);
        m_compiledPatterns.append(QRegularExpression(pattern));
        savePatterns();
        qDebug() << "SmartFilterManager: Learned pattern:" << pattern << "for engine:" << m_currentEngine;
    }
    
#ifdef HAS_PYTHON
    // AI Learn
    if (!d->m_pyFilter.is_none()) {
        try {
            py::gil_scoped_acquire acquire;
            d->m_pyFilter.attr("add_example")(text.toStdString());
            d->m_pyFilter.attr("save_state")("ai_filter_config.json");
        } catch (const std::exception &e) {
            qDebug() << "AI learn error:" << e.what();
        }
    }
#endif
}

void SmartFilterManager::unlearn(const QString &text)
{
    if (text.isEmpty()) return;

    QString pattern = QRegularExpression::escape(text);
    // Try to match the generalized pattern logic from learn()
    static QRegularExpression numberSuffix("_?\\d+$");
    if (text.contains(numberSuffix)) {
        pattern = text;
        pattern.replace(numberSuffix, "_?\\\\d+");
        pattern = QRegularExpression::escape(text);
        pattern.replace(QRegularExpression("_?\\d+$"), "_?\\\\d+");
    }

    if (m_ignoredPatterns.contains(pattern)) {
        m_ignoredPatterns.removeOne(pattern);
        
        // Rebuild compiled patterns
        m_compiledPatterns.clear();
        for (const QString &p : m_ignoredPatterns) {
            m_compiledPatterns.append(QRegularExpression(p));
        }
        
        savePatterns();
        qDebug() << "SmartFilterManager: Unlearned pattern:" << pattern << "for engine:" << m_currentEngine;
    }

#ifdef HAS_PYTHON
    // AI Unlearn
    if (!d->m_pyFilter.is_none()) {
        try {
            py::gil_scoped_acquire acquire;
            d->m_pyFilter.attr("remove_example")(text.toStdString());
            d->m_pyFilter.attr("save_state")("ai_filter_config.json");
        } catch (const std::exception &e) {
            qDebug() << "AI unlearn error:" << e.what();
        }
    }
#endif
}

void SmartFilterManager::setEngine(const QString &engineName)
{
    if (m_currentEngine != engineName) {
        m_currentEngine = engineName;
        if (m_currentEngine.isEmpty()) m_currentEngine = "Global";
        loadPatterns();
    }
}

bool SmartFilterManager::exportRules(const QString &filePath)
{
    QJsonObject rootObject;
    QSettings settings("MySoft", "NST");
    
    settings.beginGroup("SmartFilter");
    QStringList engines = settings.childGroups();
    
    for (const QString &engine : engines) {
        settings.beginGroup(engine);
        QStringList patterns = settings.value("IgnoredPatterns").toStringList();
        if (!patterns.isEmpty()) {
            QJsonArray jsonPatterns;
            for (const QString &p : patterns) jsonPatterns.append(p);
            rootObject[engine] = jsonPatterns;
        }
        settings.endGroup();
    }
    settings.endGroup();

    QJsonDocument doc(rootObject);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for export:" << filePath;
        return false;
    }
    file.write(doc.toJson());
    return true;
}

bool SmartFilterManager::importRules(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file for import:" << filePath;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return false;

    QJsonObject rootObject = doc.object();
    QSettings settings("MySoft", "NST");
    int newPatternsCount = 0;

    for (auto it = rootObject.begin(); it != rootObject.end(); ++it) {
        QString engine = it.key();
        QJsonArray jsonPatterns = it.value().toArray();
        
        // Load existing patterns for this engine
        QString key = QString("SmartFilter/%1/IgnoredPatterns").arg(engine);
        QStringList existingPatterns = settings.value(key).toStringList();
        QSet<QString> patternSet(existingPatterns.begin(), existingPatterns.end());
        
        for (const QJsonValue &val : jsonPatterns) {
            QString pattern = val.toString();
            if (!patternSet.contains(pattern)) {
                patternSet.insert(pattern);
                existingPatterns.append(pattern);
                newPatternsCount++;
            }
        }
        
        settings.setValue(key, existingPatterns);
    }
    
    // Reload patterns if we modified the current engine
    loadPatterns();
    qDebug() << "Imported" << newPatternsCount << "new patterns.";
    return true;
}

bool SmartFilterManager::shouldSkip(const QString &text) const
{
    if (text.isEmpty()) return true;

    // 1. Check Heuristics
    if (isNumericOrSymbol(text)) return true;
    if (isFilePath(text)) return true;
    if (isVariableLike(text)) return true;
    if (isCamelCase(text)) return true;
    if (isSnakeCase(text)) return true;
    if (isTagOrMarkup(text)) return true;
    if (isTechnicalString(text)) return true;
    if (isRepeatedSymbol(text)) return true;

    // 2. Check Learned Patterns
    for (const QRegularExpression &regex : m_compiledPatterns) {
        if (regex.match(text).hasMatch()) {
            return true;
        }
    }
    
#ifdef HAS_PYTHON
    // 3. Check AI
    if (m_aiEnabled && !d->m_pyFilter.is_none()) {
        try {
            py::gil_scoped_acquire acquire;
            bool skip = d->m_pyFilter.attr("predict")(text.toStdString()).cast<bool>();
            if (skip) {
                return true;
            }
        } catch (const std::exception &e) {
            // Be silent on prediction errors to avoid spam
        }
    }
#endif

    return false;
}

QList<bool> SmartFilterManager::shouldSkipBatch(const QStringList &texts) const
{
    QList<bool> results;
    if (texts.isEmpty()) return results;
    
    results.reserve(texts.size());
    
    struct Task {
        int index;
        QString text;
    };
    QList<Task> aiTasks;
    
    for (int i = 0; i < texts.size(); ++i) {
        bool skip = false;
        QString text = texts[i];
        
        // Check Heuristics
         if (isNumericOrSymbol(text) || isFilePath(text) || isVariableLike(text) || 
            isCamelCase(text) || isSnakeCase(text) || isTagOrMarkup(text) || 
            isTechnicalString(text) || isRepeatedSymbol(text)) {
            skip = true;
        } else {
             // Check Learned Regex
            for (const QRegularExpression &re : m_compiledPatterns) {
                if (re.match(text).hasMatch()) {
                    skip = true;
                    break;
                }
            }
        }
        
        results.append(skip); // Initial verdict
        
        if (!skip) {
            aiTasks.append({i, text});
        }
    }
    
#ifdef HAS_PYTHON
    // 2. AI Processing
    if (!aiTasks.isEmpty() && m_aiEnabled && !d->m_pyFilter.is_none()) {
        try {
            py::gil_scoped_acquire acquire;
            py::list pyTexts;
            for (const Task &task : aiTasks) {
                pyTexts.append(task.text.toStdString());
            }
            
            py::list pyResults = d->m_pyFilter.attr("predict_batch")(pyTexts).cast<py::list>();
            
            if (pyResults.size() == aiTasks.size()) {
                 for (int j = 0; j < aiTasks.size(); ++j) {
                     bool aiSkip = pyResults[j].cast<bool>();
                     if (aiSkip) {
                         results[aiTasks[j].index] = true;
                     }
                 }
            }
        } catch (const std::exception &e) {
            qDebug() << "AI Batch Predict Error:" << e.what();
        }
    }
#else
    Q_UNUSED(aiTasks)
#endif
    
    return results;
}

QStringList SmartFilterManager::ignoredPatterns() const
{
    return m_ignoredPatterns;
}

void SmartFilterManager::savePatterns()
{
    QSettings settings("MySoft", "NST");
    settings.setValue(QString("SmartFilter/%1/IgnoredPatterns").arg(m_currentEngine), m_ignoredPatterns);
    
    // Save AI Settings
    settings.setValue("SmartFilter/AI/Enabled", m_aiEnabled);
    settings.setValue("SmartFilter/AI/Threshold", m_aiThreshold);
}

void SmartFilterManager::loadPatterns()
{
    QSettings settings("MySoft", "NST");
    m_ignoredPatterns = settings.value(QString("SmartFilter/%1/IgnoredPatterns").arg(m_currentEngine)).toStringList();
    
    m_compiledPatterns.clear();
    for (const QString &pattern : m_ignoredPatterns) {
        m_compiledPatterns.append(QRegularExpression(pattern));
    }
    
    // Load AI Settings
    m_aiEnabled = settings.value("SmartFilter/AI/Enabled", true).toBool();
    m_aiThreshold = settings.value("SmartFilter/AI/Threshold", 0.75).toDouble();
#ifdef HAS_PYTHON
    if (!d->m_pyFilter.is_none()) {
        try {
            py::gil_scoped_acquire acquire;
            d->m_pyFilter.attr("set_threshold")(m_aiThreshold);
        } catch (...) {}
    }
#endif
}

void SmartFilterManager::setAIEnabled(bool enabled)
{
    m_aiEnabled = enabled;
    savePatterns();
}

bool SmartFilterManager::isAIEnabled() const
{
    return m_aiEnabled;
}

void SmartFilterManager::setAIThreshold(double threshold)
{
    m_aiThreshold = threshold;
#ifdef HAS_PYTHON
    if (!d->m_pyFilter.is_none()) {
        try {
            py::gil_scoped_acquire acquire;
            d->m_pyFilter.attr("set_threshold")(m_aiThreshold);
        } catch (...) {}
    }
#endif
    savePatterns();
}

double SmartFilterManager::aiThreshold() const
{
    return m_aiThreshold;
}

bool SmartFilterManager::isNumericOrSymbol(const QString &text) const
{
    // Check if text contains only non-letter characters
    static QRegularExpression regex("^[^\\p{L}]+$"); 
    return regex.match(text).hasMatch();
}

bool SmartFilterManager::isFilePath(const QString &text) const
{
    // Simple check for file extensions or path separators
    return text.contains("/") || text.contains(".png") || text.contains(".ogg") || text.contains(".json");
}

bool SmartFilterManager::isVariableLike(const QString &text) const
{
    // Check for common variable patterns like "Actor_1", "Switch_05"
    // Starts with letter, contains underscore and numbers, no spaces
    static QRegularExpression regex("^[A-Za-z]+_[0-9]+$");
    return regex.match(text).hasMatch();
}

bool SmartFilterManager::isCamelCase(const QString &text) const
{
    // No spaces, starts with letter, contains mixed case (e.g., "PlayerName", "maxHealth")
    // Must have at least one uppercase and one lowercase
    if (text.contains(" ")) return false;
    static QRegularExpression regex("^[a-zA-Z0-9]+$");
    if (!regex.match(text).hasMatch()) return false;

    bool hasUpper = false;
    bool hasLower = false;
    for (const QChar &c : text) {
        if (c.isUpper()) hasUpper = true;
        if (c.isLower()) hasLower = true;
    }
    return hasUpper && hasLower;
}

bool SmartFilterManager::isSnakeCase(const QString &text) const
{
    // Contains underscore, no spaces, mostly lowercase or uppercase (e.g., "enemy_id", "MAP_01")
    if (text.contains(" ")) return false;
    if (!text.contains("_")) return false;
    
    static QRegularExpression regex("^[a-zA-Z0-9_]+$");
    return regex.match(text).hasMatch();
}

bool SmartFilterManager::isTagOrMarkup(const QString &text) const
{
    // Enclosed in brackets <...>, [...], {...}
    static QRegularExpression regex("^(\\<.*\\>|\\[.*\\]|\\{.*\\})$");
    return regex.match(text).hasMatch();
}

bool SmartFilterManager::isTechnicalString(const QString &text) const
{
    // Mixed alphanumeric with no clear meaning, or hex codes (e.g., "0x123", "a1b2")
    // Or contains only symbols and numbers
    static QRegularExpression regex("^(0x[0-9A-Fa-f]+|[A-Za-z0-9]{1,4})$"); // Short alphanumeric codes
    if (regex.match(text).hasMatch()) return true;
    
    // Check for mostly symbols
    int symbolCount = 0;
    for (const QChar &c : text) {
        if (!c.isLetterOrNumber() && !c.isSpace()) symbolCount++;
    }
    if (text.length() > 0 && (double)symbolCount / text.length() > 0.5) return true;

    return false;
}

bool SmartFilterManager::isRepeatedSymbol(const QString &text) const
{
    // e.g., "======", "-----"
    if (text.length() < 3) return false;
    QChar first = text.at(0);
    if (first.isLetterOrNumber()) return false;
    
    for (const QChar &c : text) {
        if (c != first) return false;
    }
    return true;
}
