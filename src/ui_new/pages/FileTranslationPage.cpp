#include "FileTranslationPage.h"
#include "../viewmodels/FileTranslationViewModel.h"
#include "../components/translation/FileListView.h"
#include "../components/translation/TranslationTable.h"
#include "../components/translation/ProgressIndicator.h"
#include "../styles/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolBar>

namespace nst::ui {

FileTranslationPage::FileTranslationPage(TranslationServiceManager *translationService,
                                          QWidget *parent)
    : QWidget(parent)
{
    m_viewModel = new FileTranslationViewModel(translationService, this);
    setupUI();
    connectSignals();
}

FileTranslationPage::~FileTranslationPage() = default;

void FileTranslationPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Theme::Spacing::md(), Theme::Spacing::md(),
                                    Theme::Spacing::md(), Theme::Spacing::md());
    mainLayout->setSpacing(Theme::Spacing::md());
    
    // Header with toolbar
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    QLabel *title = new QLabel("File Translation", this);
    title->setStyleSheet(QString("font-size: 18px; font-weight: 600; color: %1;")
                         .arg(Theme::Colors::textPrimary().name()));
    headerLayout->addWidget(title);
    
    headerLayout->addStretch();
    
    // Toolbar buttons
    QPushButton *btnOpen = new QPushButton("📂 Open Project", this);
    QPushButton *btnSave = new QPushButton("💾 Save", this);
    QPushButton *btnDeploy = new QPushButton("🚀 Deploy", this);
    QPushButton *btnTranslate = new QPushButton("🌐 Translate All", this);
    
    btnOpen->setProperty("class", "secondary");
    btnSave->setProperty("class", "secondary");
    btnDeploy->setProperty("class", "secondary");
    
    headerLayout->addWidget(btnOpen);
    headerLayout->addWidget(btnSave);
    headerLayout->addWidget(btnDeploy);
    headerLayout->addWidget(btnTranslate);
    
    mainLayout->addLayout(headerLayout);
    
    // Progress indicator
    m_progress = new ProgressIndicator(this);
    mainLayout->addWidget(m_progress);
    
    // Main content splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setHandleWidth(8);
    
    // File list (left panel)
    m_fileList = new FileListView(this);
    m_fileList->setModel(m_viewModel->fileListModel());
    m_fileList->setMinimumWidth(250);
    m_fileList->setMaximumWidth(400);
    m_mainSplitter->addWidget(m_fileList);
    
    // Translation table (right panel)
    m_translationTable = new TranslationTable(this);
    m_translationTable->setModel(m_viewModel->translationModel());
    m_mainSplitter->addWidget(m_translationTable);
    
    // Splitter proportions
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 3);
    
    mainLayout->addWidget(m_mainSplitter, 1);
    
    // Connect toolbar buttons
    connect(btnOpen, &QPushButton::clicked, this, [this]() {
        // TODO: Open project dialog
    });
    
    connect(btnSave, &QPushButton::clicked, this, [this]() {
        m_viewModel->saveProject();
    });
    
    connect(btnDeploy, &QPushButton::clicked, this, [this]() {
        m_viewModel->deployProject();
    });
    
    connect(btnTranslate, &QPushButton::clicked, this, [this]() {
        m_viewModel->translateAll();
    });
}

void FileTranslationPage::connectSignals()
{
    // ViewModel -> UI
    connect(m_viewModel, &FileTranslationViewModel::isBusyChanged,
            this, [this](bool busy) {
                m_progress->setVisible(busy);
            });
    
    connect(m_viewModel, &FileTranslationViewModel::statusMessageChanged,
            m_progress, &ProgressIndicator::setMessage);
    
    connect(m_viewModel, &FileTranslationViewModel::statsChanged,
            this, [this]() {
                m_progress->setProgress(m_viewModel->progress());
            });
    
    connect(m_viewModel, &FileTranslationViewModel::projectChanged,
            this, [this]() {
                if (m_viewModel->hasProject()) {
                    emit projectLoaded(m_viewModel->projectPath());
                }
            });
    
    // UI -> ViewModel
    connect(m_fileList, &FileListView::fileSelected,
            this, [this](const QModelIndex &index) {
                m_viewModel->selectFile(index);
            });
    
    connect(m_translationTable, &TranslationTable::translationEdited,
            m_viewModel, &FileTranslationViewModel::updateTranslation);
}

void FileTranslationPage::openProject(const QString &engineName, const QString &projectPath)
{
    m_viewModel->openProject(engineName, projectPath);
}

} // namespace nst::ui
