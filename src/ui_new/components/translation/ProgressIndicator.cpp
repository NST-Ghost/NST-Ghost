#include "ProgressIndicator.h"
#include "../../styles/Theme.h"

#include <QHBoxLayout>

namespace nst::ui {

ProgressIndicator::ProgressIndicator(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setVisible(false);  // Hidden by default
}

void ProgressIndicator::setupUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(12);
    
    m_messageLabel = new QLabel(this);
    m_messageLabel->setStyleSheet(QString("color: %1; font-size: 12px;")
                                   .arg(Theme::Colors::textSecondary().name()));
    layout->addWidget(m_messageLabel);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setMinimumWidth(200);
    
    m_progressBar->setStyleSheet(QString(R"(
        QProgressBar {
            background-color: %1;
            border: none;
            border-radius: 3px;
        }
        
        QProgressBar::chunk {
            background-color: %2;
            border-radius: 3px;
        }
    )").arg(Theme::Colors::secondary().name())
       .arg(Theme::Colors::primary().name()));
    
    layout->addWidget(m_progressBar);
    layout->addStretch();
}

void ProgressIndicator::setProgress(double percent)
{
    m_progressBar->setValue(static_cast<int>(percent));
}

void ProgressIndicator::setMessage(const QString &message)
{
    m_messageLabel->setText(message);
}

void ProgressIndicator::setIndeterminate(bool indeterminate)
{
    if (indeterminate) {
        m_progressBar->setRange(0, 0);
    } else {
        m_progressBar->setRange(0, 100);
    }
}

} // namespace nst::ui
