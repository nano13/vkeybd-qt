#ifndef INTERFACE_NOTIFY_TRAY_H
#define INTERFACE_NOTIFY_TRAY_H

#include <QObject>
#include <QString>

#include "interface_notify.h"

class QSystemTrayIcon;

/*
 * Notifications as system tray messages (QSystemTrayIcon): Windows (toast),
 * macOS (Notification Center), Linux desktops with a tray. A tray shows one
 * message per icon at a time, so a new status replaces the visible one.
 * Shows nothing if the desktop has no system tray.
 */
class InterfaceNotifyTray : public InterfaceNotify
{
    Q_OBJECT
    
public:
    explicit InterfaceNotifyTray(QObject *parent = nullptr);
    
    void showStatus(const QString &title, const QString &body) override;
    
private:
    static constexpr int TIMEOUT_MS = 2500;
    
    QSystemTrayIcon *tray = nullptr;
};

#endif // INTERFACE_NOTIFY_TRAY_H
