#include "interface_notify.h"

InterfaceNotify::InterfaceNotify(QObject *parent)
    : QObject(parent)
{
    
}

void InterfaceNotify::showStatus(const QString &title, const QString &body)
{
    Q_UNUSED(title);
    Q_UNUSED(body);
}
