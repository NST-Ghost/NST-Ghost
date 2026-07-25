#include "customprogressdialog.h"

CustomProgressDialog::CustomProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Loading Game Project...");
    setWindowModality(Qt::WindowModal);

    m_labelText = new QLabel("Initializing...", this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    m_cancelButton = new QPushButton("Stop / Cancel Translation", this);
    connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
        emit canceled();
        reject();
    });

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_labelText);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_cancelButton);

    setLayout(layout);
}

CustomProgressDialog::~CustomProgressDialog()
{
}

void CustomProgressDialog::setValue(int value)
{
    m_progressBar->setValue(value);
}

void CustomProgressDialog::setLabelText(const QString &text)
{
    m_labelText->setText(text);
}

void CustomProgressDialog::setRange(int min, int max)
{
    m_progressBar->setRange(min, max);
}

void CustomProgressDialog::setMaximum(int max)
{
    m_progressBar->setMaximum(max);
}