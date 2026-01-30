#include "BaseViewModel.h"

namespace nst::ui {

BaseViewModel::BaseViewModel(QObject *parent)
    : QObject(parent)
{
}

void BaseViewModel::setBusy(bool busy)
{
    if (m_isBusy != busy) {
        m_isBusy = busy;
        emit isBusyChanged(busy);
    }
}

void BaseViewModel::setStatusMessage(const QString &message)
{
    if (m_statusMessage != message) {
        m_statusMessage = message;
        emit statusMessageChanged(message);
    }
}

void BaseViewModel::reportError(const QString &error)
{
    emit errorOccurred(error);
}

} // namespace nst::ui
