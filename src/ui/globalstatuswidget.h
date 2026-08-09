#ifndef GLOBALSTATUSWIDGET_H
#define GLOBALSTATUSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QHBoxLayout>

class GlobalStatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GlobalStatusWidget(QWidget *parent = nullptr);

public slots:
    void setTableTranslationActive(bool active);
    void updateProgress(int current, int total);
    void resetProgress();

signals:
    void tableStatusClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void updateStatusIndicator(QLabel *indicator, bool active);
    QLabel *m_tableIndicator;
    QLabel *m_tableLabel;
    QProgressBar *m_miniProgressBar;
};

#endif // GLOBALSTATUSWIDGET_H
