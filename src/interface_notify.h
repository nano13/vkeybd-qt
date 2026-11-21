#ifndef INTERFACE_NOTIFY_H
#define INTERFACE_NOTIFY_H

#include <QObject>
#include <QString>

#include "enums_structs.h"

/*
 * Desktop notifications. The status notification shows the current tab and
 * key shift of every keyboard section together, in ONE notification that is
 * replaced on every change -- separate notifications would queue up behind
 * each other (e.g. on GNOME, which keeps them on screen for a long time).
 *
 * Backends: InterfaceNotifyDBus (Linux, org.freedesktop.Notifications),
 * InterfaceNotifyTray (system tray message: Windows, macOS, Linux desktops
 * with a tray). This base class shows nothing ("none").
 */
class InterfaceNotify : public QObject
{
    Q_OBJECT
public:
    explicit InterfaceNotify(QObject *parent = nullptr);
    
    // title: one line; body: may be empty or have several lines
    virtual void showStatus(const QString &title, const QString &body);
};

#endif // INTERFACE_NOTIFY_H
