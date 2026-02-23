#ifndef GLOBALSTATUSWIDGET_H
#define GLOBALSTATUSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>

class GlobalStatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GlobalStatusWidget(QWidget *parent = nullptr);

public slots:
    void setTableTranslationActive(bool active);

signals:
    void tableStatusClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void updateStatusIndicator(QLabel *indicator, bool active);
    QLabel *m_tableIndicator;
    QLabel *m_tableLabel;
};

#endif // GLOBALSTATUSWIDGET_H
