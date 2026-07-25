#include "searchcontroller.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>

namespace {
QStringList tokenizeSearchQuery(const QString &query)
{
    QStringList tokens;
    QString current;
    QChar quote;
    bool escaping = false;
    bool inRegex = false;

    for (int i = 0; i < query.size(); ++i) {
        const QChar ch = query.at(i);

        if (escaping) {
            current.append(ch);
            escaping = false;
            continue;
        }

        if (ch == '\\') {
            escaping = true;
            current.append(ch);
            continue;
        }

        if (!quote.isNull()) {
            if (ch == quote) {
                quote = QChar();
            } else {
                current.append(ch);
            }
            continue;
        }

        if (inRegex) {
            current.append(ch);
            if (ch == '/') {
                inRegex = false;
            }
            continue;
        }

        if (ch == '"' || ch == '\'') {
            quote = ch;
            continue;
        }

        if (ch == '/' && current.isEmpty()) {
            inRegex = true;
            current.append(ch);
            continue;
        }

        if (ch.isSpace()) {
            if (!current.isEmpty()) {
                tokens.append(current);
                current.clear();
            }
            continue;
        }

        current.append(ch);
    }

    if (!current.isEmpty()) {
        tokens.append(current);
    }

    return tokens;
}

QString fieldValue(const QJsonObject &object, const QString &filePath, const QString &field)
{
    if (field == "source") return object["source"].toString();
    if (field == "translation") return object["text"].toString();
    if (field == "key") return object["key"].toString();
    if (field == "file") return QFileInfo(filePath).fileName() + "\n" + filePath;
    if (field == "warning") return object["warning"].toString();

    return QStringList({
        object["key"].toString(),
        object["source"].toString(),
        object["text"].toString(),
        object["warning"].toString(),
        QFileInfo(filePath).fileName(),
        filePath
    }).join('\n');
}

QString fieldValue(QAbstractItemModel *model, int row, const QString &field)
{
    auto textAt = [model, row](int column) {
        return model->data(model->index(row, column)).toString();
    };

    if (field == "key") return textAt(0);
    if (field == "source") return textAt(1);
    if (field == "translation") return textAt(2);
    if (field == "warning") {
        return model->data(model->index(row, 0), Qt::UserRole + 2).toString();
    }

    QStringList values;
    for (int column = 0; column < model->columnCount(); ++column) {
        values.append(textAt(column));
    }
    return values.join('\n');
}

QString normalizeField(QString field)
{
    field = field.toLower();
    if (field == "src" || field == "source") return "source";
    if (field == "tr" || field == "trans" || field == "translation" || field == "translated") return "translation";
    if (field == "ctx" || field == "context" || field == "key") return "key";
    if (field == "path" || field == "file") return "file";
    if (field == "warn" || field == "warning") return "warning";
    return QString();
}

bool isTruthy(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on" || normalized == "cs";
}
}

SearchController::SearchController(QAbstractItemModel *model, QTableView *view, QObject *parent)
    : QObject(parent), m_translationModel(model), m_view(view), m_loadedGameProjectData(nullptr), m_fileListModel(nullptr)
{
}

void SearchController::setTranslationModel(QAbstractItemModel *model)
{
    m_translationModel = model;
}

void SearchController::setLoadedGameProjectData(const QMap<QString, QJsonArray> *data)
{
    m_loadedGameProjectData = data;
}

void SearchController::setDatabase(const NstDatabase *db)
{
    m_db = db;
}

void SearchController::setFileListModel(QStandardItemModel *model)
{
    m_fileListModel = model;
}

QList<QPair<QString, QPair<int, QString>>> SearchController::searchAllFiles(const QString &query) const
{
    const ParsedQuery parsedQuery = parseQuery(query);
    if (parsedQuery.terms.isEmpty() || !parsedQuery.valid || !m_loadedGameProjectData) {
        return {};
    }

    auto searchFile = [this, parsedQuery](const QString &filePath) {
        QList<QPair<QString, QPair<int, QString>>> results;
        const QJsonArray &textsArray = m_loadedGameProjectData->value(filePath);
        int row = 0;
        for (const QJsonValue &value : textsArray) {
            if (value.isObject()) {
                QJsonObject textObject = value.toObject();
                if (matchesObject(textObject, filePath, parsedQuery)) {
                    results.append(qMakePair(filePath, qMakePair(row, buildResultPreview(textObject))));
                }
            }
            row++;
        }
        return results;
    };

    auto reduceLists = [](QList<QPair<QString, QPair<int, QString>>> &result, const QList<QPair<QString, QPair<int, QString>>> &intermediate) {
        result.append(intermediate);
    };

    return QtConcurrent::blockingMappedReduced(m_loadedGameProjectData->keys(), searchFile, reduceLists);
}

SearchController::ParsedQuery SearchController::parseQuery(const QString &query) const
{
    ParsedQuery parsed;
    const QStringList tokens = tokenizeSearchQuery(query.trimmed());

    for (QString token : tokens) {
        if (token.startsWith('-') || token.startsWith('!')) {
            token.remove(0, 1);
        }

        const int colonIndex = token.indexOf(':');
        if (colonIndex <= 0) {
            continue;
        }

        const QString prefix = token.left(colonIndex).toLower();
        if (prefix == "case" || prefix == "cs") {
            parsed.caseSensitivity = isTruthy(token.mid(colonIndex + 1)) ? Qt::CaseSensitive : Qt::CaseInsensitive;
        }
    }

    for (QString token : tokens) {
        if (token.isEmpty()) {
            continue;
        }

        bool negative = false;
        if (token.startsWith('-') || token.startsWith('!')) {
            negative = true;
            token.remove(0, 1);
        }

        const int colonIndex = token.indexOf(':');
        QString prefix;
        if (colonIndex > 0) {
            prefix = token.left(colonIndex).toLower();
            token = token.mid(colonIndex + 1);
        }

        if (prefix == "case" || prefix == "cs") {
            continue;
        }

        SearchTerm term;
        term.negative = negative;

        if (prefix == "status" || prefix == "is") {
            term.field = "status";
            term.value = token.toLower();
        } else if (prefix == "re" || prefix == "regex") {
            term.regex = true;
            term.value = token;
        } else if (!prefix.isEmpty()) {
            term.field = normalizeField(prefix);
            if (term.field.isEmpty()) {
                term.value = prefix + ":" + token;
            } else {
                term.value = token;
            }
        } else if (token.size() >= 2 && token.startsWith('/') && token.endsWith('/')) {
            term.regex = true;
            term.value = token.mid(1, token.size() - 2);
        } else {
            term.value = token;
        }

        if (term.value.isEmpty()) {
            continue;
        }

        if (term.regex) {
            QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
            if (parsed.caseSensitivity == Qt::CaseInsensitive) {
                options |= QRegularExpression::CaseInsensitiveOption;
            }
            term.expression = QRegularExpression(term.value, options);
            if (!term.expression.isValid()) {
                parsed.valid = false;
                return parsed;
            }
        }

        parsed.terms.append(term);
    }

    return parsed;
}

bool SearchController::matchesObject(const QJsonObject &object, const QString &filePath, const ParsedQuery &query) const
{
    for (const SearchTerm &term : query.terms) {
        bool matched = false;

        if (term.field == "status") {
            const bool done = !object["text"].toString().trimmed().isEmpty();
            const bool warning = !object["warning"].toString().trimmed().isEmpty();
            const QString value = term.value.toLower();
            matched = (value == "done" || value == "translated" || value == "complete") ? done
                : (value == "todo" || value == "pending" || value == "empty" || value == "untranslated") ? !done
                : (value == "warning" || value == "warn") ? warning
                : false;
        } else {
            const QString haystack = fieldValue(object, filePath, term.field);
            matched = term.regex
                ? term.expression.match(haystack).hasMatch()
                : haystack.contains(term.value, query.caseSensitivity);
        }

        if (term.negative ? matched : !matched) {
            return false;
        }
    }

    return true;
}

bool SearchController::matchesModelRow(int row, const ParsedQuery &query) const
{
    for (const SearchTerm &term : query.terms) {
        bool matched = false;

        if (term.field == "status") {
            const QString translation = fieldValue(m_translationModel, row, "translation");
            const QString warning = fieldValue(m_translationModel, row, "warning");
            const bool done = !translation.trimmed().isEmpty();
            const bool hasWarning = !warning.trimmed().isEmpty();
            const QString value = term.value.toLower();
            matched = (value == "done" || value == "translated" || value == "complete") ? done
                : (value == "todo" || value == "pending" || value == "empty" || value == "untranslated") ? !done
                : (value == "warning" || value == "warn") ? hasWarning
                : false;
        } else if (term.field == "file") {
            matched = true;
        } else {
            const QString haystack = fieldValue(m_translationModel, row, term.field);
            matched = term.regex
                ? term.expression.match(haystack).hasMatch()
                : haystack.contains(term.value, query.caseSensitivity);
        }

        if (term.negative ? matched : !matched) {
            return false;
        }
    }

    return true;
}

QString SearchController::buildResultPreview(const QJsonObject &object, const SearchTerm *matchedTerm) const
{
    Q_UNUSED(matchedTerm);

    QString source = object["source"].toString().simplified();
    QString translation = object["text"].toString().simplified();
    const QString key = object["key"].toString().simplified();

    if (source.size() > 120) source = source.left(117) + "...";
    if (translation.size() > 120) translation = translation.left(117) + "...";

    if (!source.isEmpty() && !translation.isEmpty()) {
        return QString("%1 -> %2").arg(source, translation);
    }
    if (!source.isEmpty()) {
        return source;
    }
    if (!translation.isEmpty()) {
        return translation;
    }
    return key;
}

void SearchController::setHideCompleted(bool hide)
{
    m_hideCompleted = hide;
    onSearchQueryChanged(m_currentQuery); // Re-apply filter
}

void SearchController::onSearchQueryChanged(const QString &query)
{
    m_currentQuery = query; // Update current query
    if (!m_translationModel || !m_view) {
        return;
    }

    const ParsedQuery parsedQuery = parseQuery(query);

    // Optimization: Disable updates while filtering
    m_view->setUpdatesEnabled(false);

    for (int i = 0; i < m_translationModel->rowCount(); ++i) {
        bool shouldHide = false;
        
        // Check hide completed filter first
        if (m_hideCompleted) {
             QString transText = m_translationModel->data(m_translationModel->index(i, 2)).toString();
             if (!transText.isEmpty()) {
                 shouldHide = true;
             }
        }

        if (!shouldHide) {
            if (parsedQuery.terms.isEmpty() || !parsedQuery.valid) {
                shouldHide = false;
            } else {
                shouldHide = !matchesModelRow(i, parsedQuery);
            }
        }
        
        // Only call setRowHidden if state changes to avoid unnecessary overhead
        if (m_view->isRowHidden(i) != shouldHide) {
            m_view->setRowHidden(i, shouldHide);
        }
    }
    
    m_view->setUpdatesEnabled(true);
}
