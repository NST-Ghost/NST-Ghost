#ifndef NST_UI_BASEVIEWMODEL_H
#define NST_UI_BASEVIEWMODEL_H

#include <QObject>
#include <QVariant>

namespace nst::ui {

/**
 * @brief Base class for all ViewModels in the MVVM architecture.
 * 
 * Provides common functionality for property change notifications
 * and command execution patterns.
 */
class BaseViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit BaseViewModel(QObject *parent = nullptr);
    virtual ~BaseViewModel() = default;

    bool isBusy() const { return m_isBusy; }
    QString statusMessage() const { return m_statusMessage; }

signals:
    void isBusyChanged(bool busy);
    void statusMessageChanged(const QString &message);
    void errorOccurred(const QString &error);

protected:
    void setBusy(bool busy);
    void setStatusMessage(const QString &message);
    void reportError(const QString &error);

private:
    bool m_isBusy = false;
    QString m_statusMessage;
};

} // namespace nst::ui

#endif // NST_UI_BASEVIEWMODEL_H
