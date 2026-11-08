#include "interface_notify.h"

InterfaceNotify::InterfaceNotify(QObject *parent)
    : QObject(parent)
{
    
}

void InterfaceNotify::sendNotification(const QString &message)
{
    Q_UNUSED(message);
}

int InterfaceNotify::sendNotification(const NotifyData &data)
{
    Q_UNUSED(data);
    return 0; // No notification ID from the disabled backend.
}

void InterfaceNotify::sendKeyShiftNotification(int keyshift)
{
    Q_UNUSED(keyshift);
}

void InterfaceNotify::sendTabChangeNotification(const QString &text)
{
    Q_UNUSED(text);
}
