#include "featuremanagerdialog.h"
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

#ifdef HAS_PYTHON
#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#pragma pop_macro("slots")
namespace py = pybind11;
#endif

FeatureManagerDialog::FeatureManagerDialog(QWidget *parent)
    : QDialog(parent)
    , m_installProcess(nullptr)
{
    setWindowTitle(tr("Feature Manager"));
    setMinimumWidth(450);
    setupUI();
    refreshStatus();
}

void FeatureManagerDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    
    // Header
    QLabel *headerLabel = new QLabel(tr("<h2>AI Feature Status</h2>"));
    mainLayout->addWidget(headerLabel);
    
    // Status Group
    QGroupBox *statusGroup = new QGroupBox(tr("Installed Features"));
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    
    // Python Support (compile-time)
    QHBoxLayout *pythonRow = new QHBoxLayout();
    pythonRow->addWidget(new QLabel(tr("Python Support:")));
    m_pythonStatus = new QLabel();
    pythonRow->addWidget(m_pythonStatus);
    pythonRow->addStretch();
    statusLayout->addLayout(pythonRow);
    
    // EasyOCR
    QHBoxLayout *easyocrRow = new QHBoxLayout();
    easyocrRow->addWidget(new QLabel(tr("EasyOCR (OCR Engine):")));
    m_easyocrStatus = new QLabel();
    easyocrRow->addWidget(m_easyocrStatus);
    easyocrRow->addStretch();
    statusLayout->addLayout(easyocrRow);
    
    // PyTorch
    QHBoxLayout *pytorchRow = new QHBoxLayout();
    pytorchRow->addWidget(new QLabel(tr("PyTorch (AI Backend):")));
    m_pytorchStatus = new QLabel();
    pytorchRow->addWidget(m_pytorchStatus);
    pytorchRow->addStretch();
    statusLayout->addLayout(pytorchRow);
    
    // Inpainting
    QHBoxLayout *inpaintingRow = new QHBoxLayout();
    inpaintingRow->addWidget(new QLabel(tr("LaMa Inpainting:")));
    m_inpaintingStatus = new QLabel();
    inpaintingRow->addWidget(m_inpaintingStatus);
    inpaintingRow->addStretch();
    statusLayout->addLayout(inpaintingRow);
    
    mainLayout->addWidget(statusGroup);
    
    // Progress
    m_progressLabel = new QLabel();
    m_progressLabel->setVisible(false);
    mainLayout->addWidget(m_progressLabel);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 0); // Indeterminate
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_installButton = new QPushButton(tr("Install AI Features"));
    m_installButton->setIcon(QIcon::fromTheme("system-software-install"));
    connect(m_installButton, &QPushButton::clicked, this, &FeatureManagerDialog::onInstallAIFeatures);
    buttonLayout->addWidget(m_installButton);
    
    QPushButton *refreshButton = new QPushButton(tr("Refresh"));
    refreshButton->setIcon(QIcon::fromTheme("view-refresh"));
    connect(refreshButton, &QPushButton::clicked, this, &FeatureManagerDialog::refreshStatus);
    buttonLayout->addWidget(refreshButton);
    
    buttonLayout->addStretch();
    
    QPushButton *closeButton = new QPushButton(tr("Close"));
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void FeatureManagerDialog::refreshStatus()
{
    bool pythonOk = hasPythonSupport();
    bool easyocrOk = hasEasyOCR();
    bool pytorchOk = hasPyTorch();
    bool inpaintingOk = hasInpainting();
    
    m_pythonStatus->setText(getStatusIcon(pythonOk) + " " + getStatusText(pythonOk));
    m_easyocrStatus->setText(getStatusIcon(easyocrOk) + " " + getStatusText(easyocrOk));
    m_pytorchStatus->setText(getStatusIcon(pytorchOk) + " " + getStatusText(pytorchOk));
    m_inpaintingStatus->setText(getStatusIcon(inpaintingOk) + " " + getStatusText(inpaintingOk));
    
    // Enable install button only if Python is available but dependencies are missing
    bool needsInstall = pythonOk && (!easyocrOk || !pytorchOk || !inpaintingOk);
    m_installButton->setEnabled(needsInstall);
    
    if (!pythonOk) {
        m_installButton->setToolTip(tr("Python support not compiled. Rebuild with -DNST_ENABLE_PYTHON=ON"));
    } else if (!needsInstall) {
        m_installButton->setToolTip(tr("All AI features are installed"));
    } else {
        m_installButton->setToolTip(tr("Click to install missing AI features"));
    }
}

QString FeatureManagerDialog::getStatusIcon(bool installed) const
{
    return installed ? "[OK]" : "[NO]";
}

QString FeatureManagerDialog::getStatusText(bool installed) const
{
    return installed ? tr("Installed") : tr("Not Installed");
}

bool FeatureManagerDialog::hasPythonSupport()
{
#ifdef HAS_PYTHON
    return true;
#else
    return false;
#endif
}

bool FeatureManagerDialog::hasEasyOCR()
{
#ifdef HAS_PYTHON
    try {
        py::gil_scoped_acquire acquire;
        py::module_::import("easyocr");
        return true;
    } catch (...) {
        return false;
    }
#else
    return false;
#endif
}

bool FeatureManagerDialog::hasPyTorch()
{
#ifdef HAS_PYTHON
    try {
        py::gil_scoped_acquire acquire;
        py::module_::import("torch");
        return true;
    } catch (...) {
        return false;
    }
#else
    return false;
#endif
}

bool FeatureManagerDialog::hasInpainting()
{
#ifdef HAS_PYTHON
    try {
        py::gil_scoped_acquire acquire;
        py::module_::import("simple_lama_inpainting");
        return true;
    } catch (...) {
        return false;
    }
#else
    return false;
#endif
}

void FeatureManagerDialog::onInstallAIFeatures()
{
    // Find install script
    QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts/install-ai-features.sh";
    
    if (!QFile::exists(scriptPath)) {
        // Try relative path for development
        scriptPath = "scripts/install-ai-features.sh";
    }
    
    if (!QFile::exists(scriptPath)) {
        QMessageBox::warning(this, tr("Script Not Found"),
            tr("Could not find install-ai-features.sh script.\n\n"
               "Please run manually:\n"
               "pip install easyocr torch simple-lama-inpainting"));
        return;
    }
    
    m_installButton->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressLabel->setVisible(true);
    m_progressLabel->setText(tr("Installing AI features... This may take several minutes."));
    
    m_installProcess = new QProcess(this);
    connect(m_installProcess, &QProcess::readyReadStandardOutput, this, &FeatureManagerDialog::onProcessOutput);
    connect(m_installProcess, &QProcess::readyReadStandardError, this, &FeatureManagerDialog::onProcessOutput);
    connect(m_installProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FeatureManagerDialog::onProcessFinished);
    
    m_installProcess->start("/bin/bash", QStringList() << scriptPath);
}

void FeatureManagerDialog::onProcessOutput()
{
    if (!m_installProcess) return;
    
    QString output = m_installProcess->readAllStandardOutput();
    output += m_installProcess->readAllStandardError();
    
    // Show last meaningful line
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    if (!lines.isEmpty()) {
        QString lastLine = lines.last();
        if (lastLine.length() > 60) {
            lastLine = lastLine.left(57) + "...";
        }
        m_progressLabel->setText(lastLine);
    }
}

void FeatureManagerDialog::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_progressBar->setVisible(false);
    m_installButton->setEnabled(true);
    
    if (status == QProcess::NormalExit && exitCode == 0) {
        m_progressLabel->setText(tr("Installation complete! Refreshing status..."));
        refreshStatus();
        QMessageBox::information(this, tr("Success"),
            tr("AI features installed successfully.\n\n"
               "Please restart the application to use Image Translation."));
    } else {
        m_progressLabel->setText(tr("Installation failed. See console for details."));
        QMessageBox::warning(this, tr("Installation Failed"),
            tr("Failed to install AI features.\n\n"
               "Please try running the install script manually:\n"
               "bash scripts/install-ai-features.sh"));
    }
    
    m_installProcess->deleteLater();
    m_installProcess = nullptr;
}
