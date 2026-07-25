#include "smartfiltermanager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSet>
#include <QCoreApplication>

SmartFilterManager::SmartFilterManager(QObject *parent)
    : QObject(parent)
{
    loadPatterns();
}

void SmartFilterManager::learn(const QString &text)
{
    if (text.isEmpty()) return;

    QString pattern = QRegularExpression::escape(text);
    static QRegularExpression numberSuffix("_?\\d+$");
    if (text.contains(numberSuffix)) {
        pattern = QRegularExpression::escape(text);
        pattern.replace(QRegularExpression("_?\\d+$"), "_?\\\\d+");
    }

    if (!m_ignoredPatterns.contains(pattern)) {
        m_ignoredPatterns.append(pattern);
        m_compiledPatterns.append(QRegularExpression(pattern));
        savePatterns();
        qDebug() << "SmartFilterManager: Learned pattern:" << pattern << "for engine:" << m_currentEngine;
    }
}

void SmartFilterManager::unlearn(const QString &text)
{
    if (text.isEmpty()) return;

    QString pattern = QRegularExpression::escape(text);
    static QRegularExpression numberSuffix("_?\\d+$");
    if (text.contains(numberSuffix)) {
        pattern = QRegularExpression::escape(text);
        pattern.replace(QRegularExpression("_?\\d+$"), "_?\\\\d+");
    }

    if (m_ignoredPatterns.contains(pattern)) {
        m_ignoredPatterns.removeOne(pattern);
        
        m_compiledPatterns.clear();
        for (const QString &p : m_ignoredPatterns) {
            m_compiledPatterns.append(QRegularExpression(p));
        }
        
        savePatterns();
        qDebug() << "SmartFilterManager: Unlearned pattern:" << pattern << "for engine:" << m_currentEngine;
    }
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
    QSettings settings;
    
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
    QSettings settings;
    int newPatternsCount = 0;

    for (auto it = rootObject.begin(); it != rootObject.end(); ++it) {
        QString engine = it.key();
        QJsonArray jsonPatterns = it.value().toArray();
        
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
    
    loadPatterns();
    qDebug() << "Imported" << newPatternsCount << "new patterns.";
    return true;
}

bool SmartFilterManager::shouldSkip(const QString &text) const
{
    if (text.isEmpty()) return true;

    // 1. Check Global Heuristics
    if (EngineRules::isGlobalNonText(text)) return true;

    // 2. Check Engine-Specific Rules (RPG Maker, Ren'Py, Unity, Wolf RPG, etc.)
    if (m_engineFilterEnabled && EngineRules::shouldSkip(text, m_currentEngine)) {
        return true;
    }

    // 3. Check Learned Regex Patterns
    for (const QRegularExpression &regex : m_compiledPatterns) {
        if (regex.match(text).hasMatch()) {
            return true;
        }
    }

    return false;
}

QList<bool> SmartFilterManager::shouldSkipBatch(const QStringList &texts) const
{
    QList<bool> results;
    if (texts.isEmpty()) return results;
    
    results.reserve(texts.size());
    
    for (int i = 0; i < texts.size(); ++i) {
        results.append(shouldSkip(texts[i]));

        if ((i % 500) == 0) {
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }
    
    return results;
}

QStringList SmartFilterManager::ignoredPatterns() const
{
    return m_ignoredPatterns;
}

void SmartFilterManager::savePatterns()
{
    QSettings settings;
    settings.setValue(QString("SmartFilter/%1/IgnoredPatterns").arg(m_currentEngine), m_ignoredPatterns);
    settings.setValue("SmartFilter/EngineFilter/Enabled", m_engineFilterEnabled);
}

void SmartFilterManager::loadPatterns()
{
    QSettings settings;
    m_ignoredPatterns = settings.value(QString("SmartFilter/%1/IgnoredPatterns").arg(m_currentEngine)).toStringList();
    
    m_compiledPatterns.clear();
    for (const QString &pattern : m_ignoredPatterns) {
        m_compiledPatterns.append(QRegularExpression(pattern));
    }
    
    m_engineFilterEnabled = settings.value("SmartFilter/EngineFilter/Enabled", true).toBool();
}

void SmartFilterManager::setEngineFilterEnabled(bool enabled)
{
    m_engineFilterEnabled = enabled;
    savePatterns();
}

bool SmartFilterManager::isEngineFilterEnabled() const
{
    return m_engineFilterEnabled;
}
