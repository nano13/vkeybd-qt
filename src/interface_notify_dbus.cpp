#include "interface_notify_dbus.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QStringList>
#include <QVariantMap>

namespace
{
const char *SERVICE = "org.freedesktop.Notifications";
const char *PATH = "/org/freedesktop/Notifications";
const char *INTERFACE = "org.freedesktop.Notifications";
}

InterfaceNotifyDBus::InterfaceNotifyDBus(QObject *parent)
    : InterfaceNotify(parent)
{
    QDBusConnection::sessionBus().connect(SERVICE, PATH, INTERFACE, "NotificationClosed",
                                          this, SLOT(onNotificationClosed(uint,uint)));
}

void InterfaceNotifyDBus::showStatus(const QString &title, const QString &body)
{
    QVariantMap hints;
    // GNOME: not kept in the notification list, gone when it disappears
    hints["transient"] = true;
    hints["category"] = QStringLiteral("x-keydesk-midi.status");
    
    QDBusMessage message = QDBusMessage::createMethodCall(SERVICE, PATH, INTERFACE, "Notify");
    message << QStringLiteral("KeyDeskMIDI")  // app name
            << this->status_id               // replaces_id: update the visible one
            << QStringLiteral("")            // icon
            << title                         // summary
            << body                          // body
            << QStringList()                 // actions
            << hints
            << int(TIMEOUT_MS);              // expire timeout (ignored by some servers)
    
    // not blocking: this runs on every tab switch, also while playing
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *finished_watcher){
        QDBusPendingReply<uint> reply = *finished_watcher;
        if (!reply.isError())
            this->status_id = reply.value();
        finished_watcher->deleteLater();
    });
}

void InterfaceNotifyDBus::onNotificationClosed(uint id, uint reason)
{
    Q_UNUSED(reason);
    
    // closed or expired: the next status is a new notification
    if (id == this->status_id)
        this->status_id = 0;
}
