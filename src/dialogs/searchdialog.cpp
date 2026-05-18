#include "searchdialog.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QShowEvent>
#include <QMap>
#include <QPair>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>

SearchDialog::SearchDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Search");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint); // Reverted to FramelessWindowHint

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setPlaceholderText("Search text, \"phrase\", /regex/, source:, trans:, key:, file:, status:todo");

    m_resultsTreeWidget = new QTreeWidget(this);
    m_resultsTreeWidget->setHeaderLabels(QStringList() << "Matches");
    m_resultsTreeWidget->header()->setStretchLastSection(true);

    m_placeholderLabel = new QLabel("Start typing to search...", this);
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setStyleSheet("color: #808080; font-size: 12pt;");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5); // Add some margins
    layout->addWidget(m_lineEdit);
    layout->addWidget(m_placeholderLabel);
    layout->addWidget(m_resultsTreeWidget);
    setLayout(layout);

    // setFixedSize(400, 300); // Removed fixed size
    setMinimumSize(400, 100); // Set a reasonable minimum size

    // Apply styling to the dialog itself
    setStyleSheet("SearchDialog { background-color: #2d2d2d; border: 1px solid #007acc; border-radius: 5px; }"
                  "QLineEdit { padding: 5px; border: 1px solid #3a3d41; border-radius: 3px; background-color: #3a3d41; color: #d4d4d4; }"
                  "QTreeWidget { border: none; background-color: #2d2d2d; color: #d4d4d4; }"
                  "QTreeWidget::item { padding: 4px; }"
                  "QTreeWidget::item:selected { background-color: #007acc; color: #ffffff; }");

    m_animation = new QPropertyAnimation(m_placeholderLabel, "windowOpacity", this);
    m_animation->setDuration(200);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setInterval(300); // 300ms debounce interval
    m_searchTimer->setSingleShot(true);

    connect(m_lineEdit, &QLineEdit::textChanged, this, [this](const QString &query) {
        m_searchTimer->start();
        if (query.isEmpty()) {
            m_resultsTreeWidget->hide();
            m_placeholderLabel->show();
            m_animation->setStartValue(0.0);
            m_animation->setEndValue(1.0);
            m_animation->start();
        } else {
            m_placeholderLabel->hide();
        }
        adjustSize();
    });
    connect(m_searchTimer, &QTimer::timeout, this, [this]() {
        emit searchRequested(m_lineEdit->text());
    });
    connect(m_resultsTreeWidget, &QTreeWidget::itemClicked, this, &SearchDialog::onResultSelected);

    // Set tab order
    setTabOrder(m_lineEdit, m_resultsTreeWidget);
}

void SearchDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_lineEdit->hasFocus()) {
            emit searchRequested(m_lineEdit->text());
        } else if (m_resultsTreeWidget->hasFocus()) {
            QTreeWidgetItem *selectedItem = m_resultsTreeWidget->currentItem();
            if (selectedItem) {
                onResultSelected(selectedItem, 0);
            }
        }
    } else if (event->key() == Qt::Key_Down) {
        if (m_lineEdit->hasFocus() && m_resultsTreeWidget->topLevelItemCount() > 0) {
            m_resultsTreeWidget->setFocus();
            if (m_resultsTreeWidget->currentItem() == nullptr) {
                m_resultsTreeWidget->setCurrentItem(m_resultsTreeWidget->topLevelItem(0));
            }
        }
    } else if (event->key() == Qt::Key_Up) {
        if (m_resultsTreeWidget->hasFocus() && m_resultsTreeWidget->currentItem() == m_resultsTreeWidget->topLevelItem(0)) {
            m_lineEdit->setFocus();
        }
    } else {
        QDialog::keyPressEvent(event);
    }
}



void SearchDialog::displaySearchResults(const QList<QPair<QString, QPair<int, QString>>> &results, const QString &query)
{
    m_resultsTreeWidget->clear();
    m_resultsTreeWidget->hide();

    if (query.isEmpty()) {
        m_placeholderLabel->show();
        adjustSize();
        return;
    }

    m_placeholderLabel->hide();

    QMap<QString, QTreeWidgetItem*> fileItems;
    QMap<QString, int> fileCounts;

    for (const auto &result : results) {
        const QString &filePath = result.first;
        int row = result.second.first;
        const QString &matchingText = result.second.second;

        if (!fileItems.contains(filePath)) {
            QTreeWidgetItem *fileItem = new QTreeWidgetItem(m_resultsTreeWidget);
            fileItem->setText(0, QFileInfo(filePath).fileName());
            fileItem->setData(0, Qt::UserRole, filePath); // Store full path
            fileItem->setToolTip(0, filePath);
            fileItems.insert(filePath, fileItem);
        }

        fileCounts[filePath] += 1;
        QTreeWidgetItem *fileItem = fileItems.value(filePath);
        QTreeWidgetItem *matchItem = new QTreeWidgetItem(fileItem);
        matchItem->setText(0, QString("#%1  %2").arg(row + 1).arg(matchingText));
        matchItem->setData(0, Qt::UserRole, row); // Store row for later use
    }

    for (auto it = fileItems.constBegin(); it != fileItems.constEnd(); ++it) {
        it.value()->setText(0, QString("%1 (%2)").arg(QFileInfo(it.key()).fileName()).arg(fileCounts.value(it.key())));
    }

    if (m_resultsTreeWidget->topLevelItemCount() > 0) {
        m_resultsTreeWidget->show();
        m_resultsTreeWidget->expandAll();
        m_resultsTreeWidget->resizeColumnToContents(0);
    } else {
        // No results found, display a message
        QTreeWidgetItem *noResultsItem = new QTreeWidgetItem(m_resultsTreeWidget);
        noResultsItem->setText(0, "No results found.");
        m_resultsTreeWidget->show();
    }

    adjustSize(); // Adjust size after populating results
}

void SearchDialog::onResultSelected(QTreeWidgetItem *item, int column)
{
    if (item->parent()) { // It's a match item, not a file item
        QString fileName = item->parent()->data(0, Qt::UserRole).toString(); // Full path
        int row = item->data(0, Qt::UserRole).toInt();
        emit resultSelected(fileName, row);
        close(); // Close the dialog after selecting a result
    } else { // It's a file item, just expand/collapse
        item->setExpanded(!item->isExpanded());
    }
}







