#include "ImageTranslationPage.h"
#include "../viewmodels/ImageTranslationViewModel.h"
#include "../components/image/ImageViewer.h"
#include "../components/image/ImageQueue.h"
#include "../styles/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QButtonGroup>

namespace nst::ui {

ImageTranslationPage::ImageTranslationPage(TranslationServiceManager *translationService,
                                            QWidget *parent)
    : QWidget(parent)
{
    m_viewModel = new ImageTranslationViewModel(translationService, this);
    setupUI();
    connectSignals();
}

ImageTranslationPage::~ImageTranslationPage() = default;

void ImageTranslationPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Theme::Spacing::md(), Theme::Spacing::md(),
                                    Theme::Spacing::md(), Theme::Spacing::md());
    mainLayout->setSpacing(Theme::Spacing::md());
    
    // Header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    QLabel *title = new QLabel("Image Translation", this);
    title->setStyleSheet(QString("font-size: 18px; font-weight: 600; color: %1;")
                         .arg(Theme::Colors::textPrimary().name()));
    headerLayout->addWidget(title);
    
    headerLayout->addStretch();
    
    // Source language selector
    QLabel *srcLabel = new QLabel("Source:", this);
    srcLabel->setStyleSheet(QString("color: %1;").arg(Theme::Colors::textSecondary().name()));
    headerLayout->addWidget(srcLabel);
    
    QComboBox *srcLangCombo = new QComboBox(this);
    srcLangCombo->addItem("Japanese", "ja");
    srcLangCombo->addItem("English", "en");
    srcLangCombo->addItem("Chinese", "ch_sim");
    srcLangCombo->addItem("Korean", "ko");
    headerLayout->addWidget(srcLangCombo);
    
    headerLayout->addSpacing(16);
    
    // Buttons
    QPushButton *btnAdd = new QPushButton("➕ Add Images", this);
    QPushButton *btnTranslate = new QPushButton("🌐 Translate", this);
    QPushButton *btnTranslateAll = new QPushButton("🌐 Translate All", this);
    QPushButton *btnSave = new QPushButton("💾 Save", this);
    
    btnAdd->setProperty("class", "secondary");
    btnSave->setProperty("class", "secondary");
    
    headerLayout->addWidget(btnAdd);
    headerLayout->addWidget(btnTranslate);
    headerLayout->addWidget(btnTranslateAll);
    headerLayout->addWidget(btnSave);
    
    mainLayout->addLayout(headerLayout);
    
    // Main content splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setHandleWidth(8);
    
    // Image queue (left panel)
    m_imageQueue = new ImageQueue(this);
    m_imageQueue->setMinimumWidth(200);
    m_imageQueue->setMaximumWidth(350);
    m_mainSplitter->addWidget(m_imageQueue);
    
    // Image viewer with controls (center)
    QWidget *viewerContainer = new QWidget(this);
    QVBoxLayout *viewerLayout = new QVBoxLayout(viewerContainer);
    viewerLayout->setContentsMargins(0, 0, 0, 0);
    viewerLayout->setSpacing(8);
    
    // View mode buttons
    QHBoxLayout *viewModeLayout = new QHBoxLayout();
    QButtonGroup *viewModeGroup = new QButtonGroup(this);
    
    QPushButton *btnOriginal = new QPushButton("Original", this);
    QPushButton *btnClean = new QPushButton("Clean", this);
    QPushButton *btnTranslated = new QPushButton("Translated", this);
    
    btnOriginal->setCheckable(true);
    btnClean->setCheckable(true);
    btnTranslated->setCheckable(true);
    btnOriginal->setChecked(true);
    
    viewModeGroup->addButton(btnOriginal, 0);
    viewModeGroup->addButton(btnClean, 1);
    viewModeGroup->addButton(btnTranslated, 2);
    viewModeGroup->setExclusive(true);
    
    viewModeLayout->addWidget(btnOriginal);
    viewModeLayout->addWidget(btnClean);
    viewModeLayout->addWidget(btnTranslated);
    viewModeLayout->addStretch();
    
    QPushButton *btnPeek = new QPushButton("👁 Peek Original", this);
    btnPeek->setProperty("class", "secondary");
    viewModeLayout->addWidget(btnPeek);
    
    viewerLayout->addLayout(viewModeLayout);
    
    // Image viewer
    m_imageViewer = new ImageViewer(this);
    viewerLayout->addWidget(m_imageViewer, 1);
    
    m_mainSplitter->addWidget(viewerContainer);
    
    // Splitter proportions
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 4);
    
    mainLayout->addWidget(m_mainSplitter, 1);
    
    // Connect UI actions
    connect(btnAdd, &QPushButton::clicked, m_viewModel, &ImageTranslationViewModel::addImages);
    connect(btnTranslate, &QPushButton::clicked, m_viewModel, &ImageTranslationViewModel::translateCurrent);
    connect(btnTranslateAll, &QPushButton::clicked, m_viewModel, &ImageTranslationViewModel::translateAll);
    connect(btnSave, &QPushButton::clicked, m_viewModel, &ImageTranslationViewModel::saveCurrentImage);
    
    connect(srcLangCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, srcLangCombo](int) {
        m_viewModel->setSourceLanguage(srcLangCombo->currentData().toString());
    });
    
    connect(viewModeGroup, QOverload<int>::of(&QButtonGroup::idClicked),
            m_viewModel, &ImageTranslationViewModel::setViewMode);
    
    connect(btnPeek, &QPushButton::pressed, m_viewModel, &ImageTranslationViewModel::peekOriginal);
    connect(btnPeek, &QPushButton::released, m_viewModel, &ImageTranslationViewModel::restoreView);
}

void ImageTranslationPage::connectSignals()
{
    // ViewModel -> UI
    connect(m_viewModel, &ImageTranslationViewModel::currentImageChanged,
            m_imageViewer, &ImageViewer::setImage);
    
    connect(m_viewModel, &ImageTranslationViewModel::detectionsChanged,
            m_imageViewer, &ImageViewer::setDetections);
    
    connect(m_viewModel, &ImageTranslationViewModel::statusMessageChanged,
            this, [this](const QString &msg) {
                // TODO: Show status somewhere
                Q_UNUSED(msg);
            });
    
    // UI -> ViewModel
    connect(m_imageQueue, &ImageQueue::imageSelected,
            m_viewModel, &ImageTranslationViewModel::selectImage);
    
    connect(m_imageQueue, &ImageQueue::imageRemoved,
            m_viewModel, &ImageTranslationViewModel::removeImage);
}

} // namespace nst::ui
