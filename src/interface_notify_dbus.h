#ifndef INTERFACE_NOTIFY_DBUS_H
#define INTERFACE_NOTIFY_DBUS_H

#include <QObject>
#include <QString>

#include "interface_notify.h"

/*
 * Notifications via org.freedesktop.Notifications (Linux desktops).
 * The status notification always replaces the previous one (same id), is
 * transient (GNOME does not keep it in the notification list) and expires
 * after a short time where the server allows it.
 */
class InterfaceNotifyDBus : public InterfaceNotify
{
    Q_OBJECT
    
public:
    explicit InterfaceNotifyDBus(QObject *parent = nullptr);
    
    void showStatus(const QString &title, const QString &body) override;
    
private slots:
    void onNotificationClosed(uint id, uint reason);
    
private:
    static constexpr int TIMEOUT_MS = 2500;
    
    // id of the visible status notification, 0 = none
    uint status_id = 0;
};

#endif // INTERFACE_NOTIFY_DBUS_H
