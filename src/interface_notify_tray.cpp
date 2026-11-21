#include "interface_notify_tray.h"

#include <QDebug>
#include <QIcon>
#include <QSystemTrayIcon>

InterfaceNotifyTray::InterfaceNotifyTray(QObject *parent)
    : InterfaceNotify(parent)
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        qWarning() << "No system tray: notifications are not shown";
        return;
    }
    
    this->tray = new QSystemTrayIcon(QIcon(":icon"), this);
    this->tray->setToolTip("vkeybd-qt");
    // messages need a visible tray icon
    this->tray->show();
}

void InterfaceNotifyTray::showStatus(const QString &title, const QString &body)
{
    if (!this->tray)
        return;
    
    this->tray->showMessage(title, body, QSystemTrayIcon::NoIcon, TIMEOUT_MS);
    this->tray->setToolTip("vkeybd-qt\n" + (body.isEmpty() ? title : body));
}
