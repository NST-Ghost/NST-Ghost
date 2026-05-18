#ifndef PROJECTMANAGERWIDGET_H
#define PROJECTMANAGERWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QList>
#include <QPropertyAnimation>

#include "projectregistry.h"

/**
 * @class ProjectManagerWidget
 * @brief DaVinci Resolve-inspired project manager shown at application startup.
 *
 * Displays all registered .nst translation projects as visual cards in a
 * responsive grid layout. Users can create new projects, import existing ones,
 * open, rename, and delete projects from this central hub.
 */
class ProjectManagerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectManagerWidget(ProjectRegistry *registry, QWidget *parent = nullptr);

signals:
    /// Emitted when user wants to open an existing project file.
    void projectSelected(const QString &nstFilePath);

    /// Emitted when user wants to create a new project (show LoadProjectDialog).
    void newProjectRequested();

    /// Emitted when user imports a .nst file from filesystem.
    void projectImported(const QString &nstFilePath);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onSearchTextChanged(const QString &text);
    void onNewProjectClicked();
    void onImportClicked();
    void onOpenClicked();
    void onProjectsChanged();

private:
    /* =========================================================================
     *  UI Construction
     * ========================================================================= */
    void setupUI();
    void setupTopBar();
    void setupCardArea();
    void setupBottomBar();
    void rebuildCards();

    /* =========================================================================
     *  Card Management
     * ========================================================================= */
    QFrame *createProjectCard(const ProjectRegistry::ProjectEntry &entry);
    QPixmap loadEngineIcon(const QString &engineName) const;
    QString formatDate(const QDateTime &dt) const;
    void selectCard(QFrame *card);
    void showCardContextMenu(QFrame *card, const QPoint &globalPos);

    /* =========================================================================
     *  Members
     * ========================================================================= */
    ProjectRegistry *m_registry;

    // Top bar
    QLabel *m_titleLabel;
    QLineEdit *m_searchEdit;

    // Card area
    QScrollArea *m_scrollArea;
    QWidget *m_cardContainer;
    QGridLayout *m_cardLayout;
    QList<QFrame *> m_cards;

    // Bottom bar
    QPushButton *m_importButton;
    QPushButton *m_newProjectButton;
    QPushButton *m_openButton;

    // State
    QFrame *m_selectedCard = nullptr;
    QString m_selectedProjectPath;
    QString m_currentFilter;

    // Layout constants
    static constexpr int CARD_WIDTH = 180;
    static constexpr int CARD_HEIGHT = 220;
    static constexpr int CARD_SPACING = 16;
};

#endif // PROJECTMANAGERWIDGET_H
