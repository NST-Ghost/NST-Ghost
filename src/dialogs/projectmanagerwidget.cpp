#include "projectmanagerwidget.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QApplication>
#include <QScreen>
#include <QGraphicsDropShadowEffect>
#include <QProgressBar>
#include <QDebug>

#include <QClipboard>

/* =========================================================================
 *  Constructor
 * ========================================================================= */

ProjectManagerWidget::ProjectManagerWidget(ProjectRegistry *registry, QWidget *parent)
    : QWidget(parent)
    , m_registry(registry)
{
    setupUI();

    connect(m_registry, &ProjectRegistry::projectsChanged,
            this, &ProjectManagerWidget::onProjectsChanged);

    // Initial build
    m_registry->refreshAll();
}

/* =========================================================================
 *  UI SETUP
 * ========================================================================= */

void ProjectManagerWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupTopBar();
    setupCardArea();
    setupBottomBar();

    // Top bar
    mainLayout->addWidget(m_titleLabel->parentWidget()); // topBar widget
    // Card area
    mainLayout->addWidget(m_scrollArea, 1);
    // Bottom bar
    mainLayout->addWidget(m_openButton->parentWidget()); // bottomBar widget
}

void ProjectManagerWidget::setupTopBar()
{
    QWidget *topBar = new QWidget(this);
    topBar->setObjectName("pmTopBar");
    topBar->setFixedHeight(56);
    topBar->setStyleSheet(
        "#pmTopBar { background-color: #0d0d0d; border-bottom: 1px solid #252525; }");

    QHBoxLayout *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->setSpacing(16);

    // App icon
    QLabel *iconLabel = new QLabel();
    QPixmap appIcon(":/icons/icon-app.png");
    if (!appIcon.isNull()) {
        iconLabel->setPixmap(appIcon.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    iconLabel->setStyleSheet("background: transparent;");
    layout->addWidget(iconLabel);

    // Title
    m_titleLabel = new QLabel("NST Project Manager");
    m_titleLabel->setStyleSheet(
        "font-size: 16px; font-weight: bold; color: #ffffff; background: transparent;");
    layout->addWidget(m_titleLabel);

    layout->addStretch();

    // Search
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search projects...");
    m_searchEdit->setFixedWidth(260);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background-color: #1a1a1a; border: 1px solid #3a3a3a; "
        "border-radius: 6px; padding: 8px 12px; color: #cccccc; font-size: 12px; }"
        "QLineEdit:focus { border: 2px solid #3399ff; padding: 7px 11px; }");
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &ProjectManagerWidget::onSearchTextChanged);
    layout->addWidget(m_searchEdit);
}

void ProjectManagerWidget::setupCardArea()
{
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet(
        "QScrollArea { background-color: #1a1a1a; border: none; }"
        "QScrollArea > QWidget > QWidget { background-color: #1a1a1a; }");

    m_cardContainer = new QWidget();
    m_cardContainer->setStyleSheet("background-color: #1a1a1a;");

    m_cardLayout = new QGridLayout(m_cardContainer);
    m_cardLayout->setContentsMargins(32, 32, 32, 32);
    m_cardLayout->setSpacing(CARD_SPACING);

    m_scrollArea->setWidget(m_cardContainer);
}

void ProjectManagerWidget::setupBottomBar()
{
    QWidget *bottomBar = new QWidget(this);
    bottomBar->setObjectName("pmBottomBar");
    bottomBar->setFixedHeight(60);
    bottomBar->setStyleSheet(
        "#pmBottomBar { background-color: #0d0d0d; border-top: 1px solid #252525; }");

    QHBoxLayout *layout = new QHBoxLayout(bottomBar);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->setSpacing(12);

    // Import button
    m_importButton = new QPushButton("Import");
    m_importButton->setStyleSheet(
        "QPushButton { background-color: #3a3a3a; border: 1px solid #4a4a4a; "
        "border-radius: 6px; padding: 10px 28px; color: #cccccc; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background-color: #4a4a4a; border-color: #5a5a5a; }"
        "QPushButton:pressed { background-color: #3399ff; color: #ffffff; }");
    connect(m_importButton, &QPushButton::clicked, this, &ProjectManagerWidget::onImportClicked);
    layout->addWidget(m_importButton);

    layout->addStretch();

    // New Project button
    m_newProjectButton = new QPushButton("New Project");
    m_newProjectButton->setStyleSheet(
        "QPushButton { background-color: #3a3a3a; border: 1px solid #4a4a4a; "
        "border-radius: 6px; padding: 10px 28px; color: #cccccc; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background-color: #4a4a4a; border-color: #5a5a5a; }"
        "QPushButton:pressed { background-color: #3399ff; color: #ffffff; }");
    connect(m_newProjectButton, &QPushButton::clicked, this, &ProjectManagerWidget::onNewProjectClicked);
    layout->addWidget(m_newProjectButton);

    // Open button
    m_openButton = new QPushButton("Open");
    m_openButton->setStyleSheet(
        "QPushButton { background-color: #3399ff; border: none; "
        "border-radius: 6px; padding: 10px 28px; color: #ffffff; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background-color: #4da6ff; }"
        "QPushButton:pressed { background-color: #2680d9; }"
        "QPushButton:disabled { background-color: #252525; color: #666666; }");
    m_openButton->setEnabled(false);
    connect(m_openButton, &QPushButton::clicked, this, &ProjectManagerWidget::onOpenClicked);
    layout->addWidget(m_openButton);
}

/* =========================================================================
 *  CARD CREATION
 * ========================================================================= */

QFrame *ProjectManagerWidget::createProjectCard(const ProjectRegistry::ProjectEntry &entry)
{
    QFrame *card = new QFrame();
    card->setObjectName("projectCard");
    card->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
    card->setCursor(Qt::PointingHandCursor);
    card->setStyleSheet(
        "QFrame#projectCard { background-color: #2b2b2b; border: 2px solid #3a3a3a; "
        "border-radius: 8px; }"
        "QFrame#projectCard:hover { border-color: #4a4a4a; background-color: #303030; }");

    // Store project path in the card
    card->setProperty("projectPath", entry.filePath);
    card->setProperty("displayName", entry.displayName);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── Icon area ──
    QWidget *iconArea = new QWidget();
    iconArea->setFixedHeight(120);
    iconArea->setStyleSheet("background-color: #222222; border-top-left-radius: 6px; "
                            "border-top-right-radius: 6px;");

    QVBoxLayout *iconLayout = new QVBoxLayout(iconArea);
    iconLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *iconLabel = new QLabel();
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("background: transparent;");
    QPixmap icon = loadEngineIcon(entry.engineName);
    if (!icon.isNull()) {
        iconLabel->setPixmap(icon.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        iconLabel->setText("📁");
        iconLabel->setStyleSheet("background: transparent; font-size: 36px; color: #666666;");
    }
    iconLayout->addWidget(iconLabel, 0, Qt::AlignCenter);

    // Engine badge
    if (!entry.engineName.isEmpty()) {
        QLabel *engineBadge = new QLabel(entry.engineName);
        engineBadge->setAlignment(Qt::AlignCenter);
        engineBadge->setStyleSheet(
            "background-color: rgba(51, 153, 255, 0.2); color: #3399ff; "
            "font-size: 9px; font-weight: bold; padding: 2px 8px; "
            "border-radius: 3px; margin: 0px 30px;");
        iconLayout->addWidget(engineBadge, 0, Qt::AlignCenter);
    }

    layout->addWidget(iconArea);

    // ── Progress bar ──
    QProgressBar *progressBar = new QProgressBar();
    progressBar->setFixedHeight(4);
    progressBar->setRange(0, 100);
    progressBar->setValue(entry.translatedPercent);
    progressBar->setTextVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar { background-color: #3a3a3a; border: none; border-radius: 0px; }"
        "QProgressBar::chunk { background-color: #3399ff; border-radius: 0px; }");
    layout->addWidget(progressBar);

    // ── Info area ──
    QWidget *infoArea = new QWidget();
    infoArea->setStyleSheet("background-color: #2b2b2b; border-bottom-left-radius: 6px; "
                            "border-bottom-right-radius: 6px;");

    QVBoxLayout *infoLayout = new QVBoxLayout(infoArea);
    infoLayout->setContentsMargins(10, 6, 10, 8);
    infoLayout->setSpacing(2);

    // Project name
    QLabel *nameLabel = new QLabel(entry.displayName);
    nameLabel->setStyleSheet(
        "color: #ffffff; font-size: 11px; font-weight: bold; background: transparent;");
    nameLabel->setWordWrap(false);
    nameLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // Elide long names
    QFontMetrics fm(nameLabel->font());
    nameLabel->setText(fm.elidedText(entry.displayName, Qt::ElideRight, CARD_WIDTH - 24));
    nameLabel->setToolTip(entry.displayName);
    infoLayout->addWidget(nameLabel);

    // Date + progress text
    QString dateStr = formatDate(entry.lastModified);
    QString progressStr = QString("%1%").arg(entry.translatedPercent);
    QLabel *metaLabel = new QLabel(dateStr + "  •  " + progressStr);
    metaLabel->setStyleSheet(
        "color: #777777; font-size: 9px; background: transparent;");
    metaLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    infoLayout->addWidget(metaLabel);

    // File count
    QLabel *filesLabel = new QLabel(QString("%1 files").arg(entry.fileCount));
    filesLabel->setStyleSheet(
        "color: #555555; font-size: 9px; background: transparent;");
    filesLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    infoLayout->addWidget(filesLabel);

    layout->addWidget(infoArea);

    // ── Drop shadow ──
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 2);
    card->setGraphicsEffect(shadow);

    // ── Event handling via event filter ──
    card->installEventFilter(this->parent() ? this : this);

    return card;
}

QPixmap ProjectManagerWidget::loadEngineIcon(const QString &engineName) const
{
    QString upper = engineName.toUpper();
    if (upper == "RPGM") return QPixmap(":/ui/icons/rpgm_icon.png");
    if (upper == "UNITY") return QPixmap(":/ui/icons/unity_icon.png");
    if (upper == "RENPY") return QPixmap(":/ui/icons/renpy_icon.png");
    return QPixmap();
}

QString ProjectManagerWidget::formatDate(const QDateTime &dt) const
{
    if (!dt.isValid()) return "Unknown";

    QDateTime now = QDateTime::currentDateTime();
    qint64 daysAgo = dt.daysTo(now);

    if (daysAgo == 0) return "Today";
    if (daysAgo == 1) return "Yesterday";
    if (daysAgo < 7) return QString("%1 days ago").arg(daysAgo);
    if (daysAgo < 30) return QString("%1 weeks ago").arg(daysAgo / 7);

    return dt.toString("yyyy-MM-dd");
}

/* =========================================================================
 *  CARD GRID MANAGEMENT
 * ========================================================================= */

void ProjectManagerWidget::rebuildCards()
{
    // Clear existing cards
    for (QFrame *card : m_cards) {
        m_cardLayout->removeWidget(card);
        card->deleteLater();
    }
    m_cards.clear();
    m_selectedCard = nullptr;
    m_selectedProjectPath.clear();
    m_openButton->setEnabled(false);

    // Get filtered project list
    QList<ProjectRegistry::ProjectEntry> projects = m_registry->getAllProjects();

    // Apply search filter
    if (!m_currentFilter.isEmpty()) {
        QList<ProjectRegistry::ProjectEntry> filtered;
        for (const auto &entry : projects) {
            if (entry.displayName.contains(m_currentFilter, Qt::CaseInsensitive) ||
                entry.engineName.contains(m_currentFilter, Qt::CaseInsensitive) ||
                entry.projectPath.contains(m_currentFilter, Qt::CaseInsensitive)) {
                filtered.append(entry);
            }
        }
        projects = filtered;
    }

    // Calculate columns based on available width
    int availableWidth = m_scrollArea->viewport()->width() - 64; // margins
    int columns = qMax(1, availableWidth / (CARD_WIDTH + CARD_SPACING));

    // Create cards
    int row = 0, col = 0;
    for (const auto &entry : projects) {
        QFrame *card = createProjectCard(entry);
        m_cardLayout->addWidget(card, row, col);
        m_cards.append(card);

        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    }

    // Show empty state if no projects
    if (projects.isEmpty() && m_currentFilter.isEmpty()) {
        m_cardLayout->setAlignment(Qt::AlignCenter);
        QLabel *emptyLabel = new QLabel("No projects yet.\nClick 'New Project' to get started,\nor 'Import' to open an existing .nst file.");
        emptyLabel->setObjectName("projectCard"); // reuse name for cleanup
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(
            "color: #666666; font-size: 14px; background: transparent; padding: 60px;");
        m_cardLayout->addWidget(emptyLabel, 0, 0, 1, qMax(1, columns));

    } else if (projects.isEmpty()) {
        m_cardLayout->setAlignment(Qt::AlignCenter);
        QLabel *noResults = new QLabel(QString("No projects matching \"%1\"").arg(m_currentFilter));
        noResults->setAlignment(Qt::AlignCenter);
        noResults->setStyleSheet(
            "color: #666666; font-size: 14px; background: transparent; padding: 60px;");
        m_cardLayout->addWidget(noResults, 0, 0, 1, qMax(1, columns));
    } else {
        m_cardLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    }
}

void ProjectManagerWidget::selectCard(QFrame *card)
{
    // Deselect previous
    if (m_selectedCard) {
        m_selectedCard->setStyleSheet(
            "QFrame#projectCard { background-color: #2b2b2b; border: 2px solid #3a3a3a; "
            "border-radius: 8px; }"
            "QFrame#projectCard:hover { border-color: #4a4a4a; background-color: #303030; }");
    }

    m_selectedCard = card;
    m_selectedProjectPath = card->property("projectPath").toString();

    // Highlight selected
    card->setStyleSheet(
        "QFrame#projectCard { background-color: #2b2b2b; border: 3px solid #3399ff; "
        "border-radius: 8px; }"
        "QFrame#projectCard:hover { border-color: #4da6ff; background-color: #303030; }");

    m_openButton->setEnabled(true);
}

void ProjectManagerWidget::showCardContextMenu(QFrame *card, const QPoint &globalPos)
{
    QString filePath = card->property("projectPath").toString();
    QString displayName = card->property("displayName").toString();

    QMenu menu(this);

    QAction *openAction = menu.addAction("Open Project");
    openAction->setIcon(QIcon::fromTheme("document-open"));

    menu.addSeparator();

    QAction *renameAction = menu.addAction("Rename...");
    QAction *removeAction = menu.addAction("Remove from List");
    removeAction->setIcon(QIcon::fromTheme("list-remove"));

    QAction *deleteAction = menu.addAction("Delete Project File...");
    deleteAction->setIcon(QIcon::fromTheme("edit-delete"));

    menu.addSeparator();

    QAction *showInFolderAction = menu.addAction("Show in File Manager");
    QAction *copyPathAction = menu.addAction("Copy Path");

    QAction *chosen = menu.exec(globalPos);
    if (!chosen) return;

    if (chosen == openAction) {
        emit projectSelected(filePath);
    } else if (chosen == renameAction) {
        bool ok;
        QString newName = QInputDialog::getText(this, "Rename Project",
                                                 "New name:", QLineEdit::Normal,
                                                 displayName, &ok);
        if (ok && !newName.isEmpty()) {
            m_registry->renameProject(filePath, newName);
        }
    } else if (chosen == removeAction) {
        m_registry->removeProject(filePath);
    } else if (chosen == deleteAction) {
        int ret = QMessageBox::warning(this, "Delete Project",
                                        QString("Are you sure you want to permanently delete:\n%1\n\n"
                                                "This cannot be undone!").arg(filePath),
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            QFile::remove(filePath);
            m_registry->removeProject(filePath);
        }
    } else if (chosen == showInFolderAction) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
    } else if (chosen == copyPathAction) {
        QApplication::clipboard()->setText(filePath);
    }
}

/* =========================================================================
 *  EVENT HANDLING
 * ========================================================================= */

void ProjectManagerWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Rebuild grid to adjust column count
    rebuildCards();
}

bool ProjectManagerWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        QFrame *card = qobject_cast<QFrame*>(obj);
        if (card && card->objectName() == "projectCard") {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            
            if (event->type() == QEvent::MouseButtonPress) {
                if (mouseEvent->button() == Qt::LeftButton) {
                    selectCard(card);
                    return true;
                } else if (mouseEvent->button() == Qt::RightButton) {
                    selectCard(card);
                    showCardContextMenu(card, mouseEvent->globalPosition().toPoint());
                    return true;
                }
            } else if (event->type() == QEvent::MouseButtonDblClick) {
                if (mouseEvent->button() == Qt::LeftButton) {
                    selectCard(card);
                    onOpenClicked();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

/* =========================================================================
 *  SLOTS
 * ========================================================================= */

void ProjectManagerWidget::onSearchTextChanged(const QString &text)
{
    m_currentFilter = text.trimmed();
    rebuildCards();
}

void ProjectManagerWidget::onNewProjectClicked()
{
    emit newProjectRequested();
}

void ProjectManagerWidget::onImportClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Import Project"),
                                                     QString(),
                                                     tr("NST Workspace Files (*.nst)"));
    if (filePath.isEmpty()) return;

    if (m_registry->registerProject(filePath)) {
        emit projectImported(filePath);
    } else {
        QMessageBox::warning(this, tr("Import Failed"),
                             tr("Failed to import project file:\n%1").arg(filePath));
    }
}

void ProjectManagerWidget::onOpenClicked()
{
    if (!m_selectedProjectPath.isEmpty()) {
        emit projectSelected(m_selectedProjectPath);
    }
}

void ProjectManagerWidget::onProjectsChanged()
{
    rebuildCards();
}
