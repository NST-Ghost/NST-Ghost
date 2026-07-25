// loadprojectdialog.cpp
#include "loadprojectdialog.h"
#include "ui_loadprojectdialog.h"

#include <QFileDialog>
#include <QScrollArea>
#include <QButtonGroup>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QStyle>

LoadProjectDialog::LoadProjectDialog(const QStringList &availableEngines, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoadProjectDialog)
{
    ui->setupUi(this);
    setWindowTitle("Load from Game Project");
    setMinimumSize(700, 500);

    setupMainLayout(availableEngines);
    setupPathSection();
    setupButtonBox();

    // Connect signals
    connect(ui->browseButton, &QPushButton::clicked, this, &LoadProjectDialog::browseProjectPath);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LoadProjectDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &LoadProjectDialog::reject);

    // Global background style
    /*setStyleSheet(
        "QDialog { background-color: #2d2d2d; }"
        "QScrollBar:vertical { background: #2d2d2d; width: 12px; border-radius: 6px; }"
        "QScrollBar::handle:vertical { background: #3a3d41; border-radius: 6px; min-height: 20px; }"
        "QScrollBar::handle:vertical:hover { background: #4a4d51; }"
        );*/
}

LoadProjectDialog::~LoadProjectDialog()
{
    delete ui;
}

/* =========================================================================
 *  UI SETUP HELPERS
 * ========================================================================= */

void LoadProjectDialog::setupMainLayout(const QStringList &availableEngines)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(ui->contentWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // Title
    QLabel *titleLabel = new QLabel("Select Game Engine");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #ffffff;");
    mainLayout->addWidget(titleLabel);

    // Scroll area setup
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget *scrollContent = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setSpacing(15);
    scrollLayout->setContentsMargins(0, 0, 0, 0);

    // Engine buttons setup
    engineButtons = new QButtonGroup(this);
    engineButtons->setExclusive(true);

    for (int i = 0; i < availableEngines.size(); ++i) {
        const QString &engine = availableEngines[i];
        QFrame *engineFrame = createEngineFrame(engine, i);
        scrollLayout->addWidget(engineFrame);
        engineFrames.append(engineFrame);
    }

    scrollLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);

    // Select first engine by default
    if (!engineButtons->buttons().isEmpty()) {
        auto *firstBtn = engineButtons->button(0);
        firstBtn->setChecked(true);
        updateFrameSelection(firstBtn);
    }

    connect(engineButtons, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &LoadProjectDialog::updateFrameSelection);
}

void LoadProjectDialog::setupPathSection()
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(ui->contentWidget->layout());
    if (!layout) {
        layout = new QVBoxLayout(ui->contentWidget);
    }

    QLabel *pathLabel = new QLabel("Project Directory");
    pathLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");
    layout->addWidget(pathLabel);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->setSpacing(10);

    ui->projectPathLineEdit->setPlaceholderText("Select game project directory...");
    ui->projectPathLineEdit->setStyleSheet(
        "QLineEdit { padding: 12px; border: 2px solid #3a3d41; border-radius: 6px; "
        "background-color: #1e1e1e; color: #d4d4d4; font-size: 14px; }"
        "QLineEdit:focus { border-color: #007acc; }"
        );

    ui->browseButton->setText("Browse...");
    ui->browseButton->setStyleSheet(
        "QPushButton { padding: 12px 30px; border: none; border-radius: 6px; "
        "background-color: #007acc; color: #ffffff; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background-color: #005f99; }"
        "QPushButton:pressed { background-color: #004578; }"
        );

    pathLayout->addWidget(ui->projectPathLineEdit);
    pathLayout->addWidget(ui->browseButton);
    layout->addLayout(pathLayout);
}

void LoadProjectDialog::setupButtonBox()
{
    ui->buttonBox->setStyleSheet(
        "QPushButton { padding: 10px 30px; border-radius: 6px; font-weight: bold; font-size: 14px; }"
        "QPushButton[text='OK'] { background-color: #007acc; color: #fff; border: none; }"
        "QPushButton[text='OK']:hover { background-color: #005f99; }"
        "QPushButton[text='Cancel'] { background-color: #3a3d41; color: #d4d4d4; border: none; }"
        "QPushButton[text='Cancel']:hover { background-color: #4a4d51; }"
        );
}

/* =========================================================================
 *  ENGINE ITEM CREATION
 * ========================================================================= */

QFrame *LoadProjectDialog::createEngineFrame(const QString &engine, int index)
{
    QString engineUpper = engine.toUpper();

    QFrame *frame = new QFrame();
    frame->setObjectName("engineFrame");
    frame->setMinimumHeight(120);
    frame->setStyleSheet(defaultEngineFrameStyle());
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);

    QHBoxLayout *layout = new QHBoxLayout(frame);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    // Hidden radio button for selection tracking
    QRadioButton *radioBtn = new QRadioButton();
    radioBtn->hide();
    engineButtons->addButton(radioBtn, index);
    layout->addWidget(radioBtn);

    // Engine icon
    QLabel *iconLabel = new QLabel();
    QPixmap icon = loadEngineIcon(engineUpper);
    if (!icon.isNull()) {
        iconLabel->setPixmap(icon.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    QWidget *imageContainer = new QWidget();
    imageContainer->setFixedWidth(200);
    imageContainer->setStyleSheet("background-color: #2d2d2d; border-radius: 8px;");
    QVBoxLayout *imageLayout = new QVBoxLayout(imageContainer);
    imageLayout->addWidget(iconLabel, 0, Qt::AlignCenter);
    layout->addWidget(imageContainer);

    // Engine info
    QWidget *infoContainer = new QWidget();
    QVBoxLayout *infoLayout = new QVBoxLayout(infoContainer);
    infoLayout->setContentsMargins(25, 20, 20, 20);
    infoLayout->setSpacing(10);

    QLabel *nameLabel = new QLabel(engineUpper);
    nameLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #ffffff;");
    infoLayout->addWidget(nameLabel);

    QLabel *filesLabel = new QLabel(engineFileTypes(engineUpper));
    filesLabel->setStyleSheet("font-size: 14px; color: #999999;");
    filesLabel->setWordWrap(true);
    infoLayout->addWidget(filesLabel);
    infoLayout->addStretch();

    layout->addWidget(infoContainer, 1);

    frame->setProperty("radioButton", QVariant::fromValue(radioBtn));
    frame->setProperty("engineName", engineUpper);

    return frame;
}

QString LoadProjectDialog::defaultEngineFrameStyle() const
{
    return "QFrame#engineFrame { background-color: #1e1e1e; border: 2px solid #3a3d41; "
           "border-radius: 10px; }"
           "QFrame#engineFrame:hover { border-color: #007acc; background-color: #252526; }";
}

QPixmap LoadProjectDialog::loadEngineIcon(const QString &engine) const
{
    if (engine == "RPGM") return QPixmap(":/ui/icons/rpgm_icon.png");
    if (engine == "UNITY") return QPixmap(":/ui/icons/unity_icon.png");
    if (engine == "RENPY") return QPixmap(":/ui/icons/renpy_icon.png");
    return QPixmap(); // Return an empty pixmap as a safe fallback
}

QString LoadProjectDialog::engineFileTypes(const QString &engine) const
{
    static QMap<QString, QString> types{
        {"RPGM", "RPG Maker (MV/MZ) & PGMMV / Cocos2d-JS (.json, .png, .ogg, ...)"},
        {"UNITY", "unity.exe, .assets, .dll, ..."},
        {"RENPY", ".rpy, .rpyc, ..."}
    };
    return types.value(engine, "...");
}

/* =========================================================================
 *  EVENT HANDLING
 * ========================================================================= */

void LoadProjectDialog::browseProjectPath()
{
    QFileDialog dialog(this, "Select Game Project Directory", QDir::homePath());
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    
    if (dialog.exec()) {
        QStringList distinct = dialog.selectedFiles();
        if (!distinct.isEmpty()) {
            ui->projectPathLineEdit->setText(distinct.first());
        }
    }
}

void LoadProjectDialog::updateFrameSelection(QAbstractButton *button)
{
    int id = engineButtons->id(button);
    for (int i = 0; i < engineFrames.size(); ++i) {
        QFrame *frame = engineFrames[i];
        frame->setStyleSheet(
            i == id
                ? "QFrame#engineFrame { background-color: #1e1e1e; border: 3px solid #007acc; border-radius: 10px; }"
                : defaultEngineFrameStyle()
            );
    }
}

bool LoadProjectDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (auto *frame = qobject_cast<QFrame *>(obj)) {
            if (auto *radio = frame->property("radioButton").value<QRadioButton *>()) {
                radio->setChecked(true);
                updateFrameSelection(radio);
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

/* =========================================================================
 *  PUBLIC ACCESSORS
 * ========================================================================= */

QString LoadProjectDialog::selectedEngine() const
{
    int id = engineButtons->checkedId();
    if (id >= 0 && id < engineFrames.size())
        return engineFrames[id]->property("engineName").toString();
    return "";
}

QString LoadProjectDialog::projectPath() const
{
    return ui->projectPathLineEdit->text();
}
