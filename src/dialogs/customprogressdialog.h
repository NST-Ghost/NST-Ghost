#ifndef CUSTOMPROGRESSDIALOG_H
#define CUSTOMPROGRESSDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

class CustomProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomProgressDialog(QWidget *parent = nullptr);
    ~CustomProgressDialog();

public slots:
    void setValue(int value);
    void setLabelText(const QString &text);
    void setRange(int min, int max);
    void setMaximum(int max);

signals:
    void canceled();

private:
    QProgressBar *m_progressBar;
    QLabel *m_labelText;
    QPushButton *m_cancelButton;
};

#endif // CUSTOMPROGRESSDIALOG_H