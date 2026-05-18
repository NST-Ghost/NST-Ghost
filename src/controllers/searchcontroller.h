#ifndef SEARCHCONTROLLER_H
#define SEARCHCONTROLLER_H

#include <QObject>
#include <QStandardItemModel>
#include <QTableView>
#include <QJsonArray>
#include <QMap>
#include <QStringListModel>
#include <QPair>
#include <QtConcurrent/QtConcurrent>
#include <QJsonObject>
#include <QRegularExpression>

class SearchController : public QObject
{
    Q_OBJECT
public:
    explicit SearchController(QStandardItemModel *model, QTableView *view, QObject *parent = nullptr);

    void setTranslationModel(QStandardItemModel *model);
    void setLoadedGameProjectData(const QMap<QString, QJsonArray> *data);
    void setFileListModel(QStandardItemModel *model);

    QList<QPair<QString, QPair<int, QString>>> searchAllFiles(const QString &query) const;
    void setHideCompleted(bool hide); // New method
    QString currentQuery() const { return m_currentQuery; }

public slots:
    void onSearchQueryChanged(const QString &query);

private:
    struct SearchTerm {
        QString field;
        QString value;
        bool negative = false;
        bool regex = false;
        QRegularExpression expression;
    };

    struct ParsedQuery {
        QList<SearchTerm> terms;
        Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
        bool valid = true;
    };

    ParsedQuery parseQuery(const QString &query) const;
    bool matchesObject(const QJsonObject &object, const QString &filePath, const ParsedQuery &query) const;
    bool matchesModelRow(int row, const ParsedQuery &query) const;
    QString buildResultPreview(const QJsonObject &object, const SearchTerm *matchedTerm = nullptr) const;

    QStandardItemModel *m_translationModel;
    QTableView *m_view;
    const QMap<QString, QJsonArray> *m_loadedGameProjectData;
    QStandardItemModel *m_fileListModel;
    bool m_hideCompleted = false; // New member
    QString m_currentQuery; // Store current query to re-apply filter
};

#endif // SEARCHCONTROLLER_H
