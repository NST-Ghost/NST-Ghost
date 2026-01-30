#ifndef NST_UI_APPNAVIGATION_H
#define NST_UI_APPNAVIGATION_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QButtonGroup>

namespace nst::ui {

/**
 * @brief Sidebar navigation component.
 * 
 * Provides navigation buttons for switching between application pages.
 */
class AppNavigation : public QWidget
{
    Q_OBJECT

public:
    explicit AppNavigation(QWidget *parent = nullptr);

    void setActiveIndex(int index);

signals:
    void navigationRequested(int index);

private:
    void setupUI();
    QPushButton* createNavButton(const QString &text, const QString &icon);

private:
    QVBoxLayout *m_layout = nullptr;
    QButtonGroup *m_buttonGroup = nullptr;
    
    QPushButton *m_btnFileTranslation = nullptr;
    QPushButton *m_btnImageTranslation = nullptr;
    QPushButton *m_btnSettings = nullptr;
};

} // namespace nst::ui

#endif // NST_UI_APPNAVIGATION_H
