#ifndef NST_UI_PROGRESSINDICATOR_H
#define NST_UI_PROGRESSINDICATOR_H

#include <QWidget>
#include <QProgressBar>
#include <QLabel>

namespace nst::ui {

/**
 * @brief Progress indicator with message display.
 */
class ProgressIndicator : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressIndicator(QWidget *parent = nullptr);

    void setProgress(double percent);
    void setMessage(const QString &message);
    void setIndeterminate(bool indeterminate);

private:
    void setupUI();

private:
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_messageLabel = nullptr;
};

} // namespace nst::ui

#endif // NST_UI_PROGRESSINDICATOR_H
