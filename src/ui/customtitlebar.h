#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

class CustomTitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit CustomTitleBar(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setIcon(const QIcon &icon);

    void setRealTimeVisible(bool visible);

signals:
    void minimizeClicked();
    void maximizeRestoreClicked();
    void closeClicked();
    void translateModeClicked(); // Switch to File Translation
    void realTimeModeClicked();  // Switch to Real-time Translation

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
    
    // Navigation Buttons
    QPushButton *m_fileTransButton;
    QPushButton *m_realTimeButton;
    
    // Window Controls
    QPushButton *m_minimizeButton;
    QPushButton *m_maximizeButton;
    QPushButton *m_closeButton;
    QHBoxLayout *m_layout;
    
    QPoint m_clickPos;
    bool m_isDrag;
};

#endif // CUSTOMTITLEBAR_H
