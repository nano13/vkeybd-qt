#include "interface_notify_dbus.h"

InterfaceNotifyDBus::InterfaceNotifyDBus(InterfaceNotify *parent)
    : InterfaceNotify(parent)
{
    QDBusConnection::sessionBus().connect(
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "NotificationClosed",
        this,
        SLOT(onNotificationClosed(uint,uint))
        );
}

void InterfaceNotifyDBus::sendNotification(const QString &message)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "Notify"
        );
    
    msg << "vkeybd-qt"        // app name
        << uint(0)            // replaces ID
        << ""                 // icon
        << "Notification"     // title
        << message            // body
        << QStringList()      // actions
        << QVariantMap()      // hints
        << int(-1);           // timeout
    
    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
}

int InterfaceNotifyDBus::sendNotification(const NotifyData &data)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "Notify"
        );
    
    msg << QStringLiteral("vkeybd-qt")              // app name
        << static_cast<uint32_t>(data.id)           // replaces_id
        << data.icon                                // icon
        << data.title                              // summary
        << data.body                             // body
        << data.actions                           // actions
        << data.hints                             // hints
        << data.timeout;                          // timeout
    
    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
    return 0; // Asynchronous DBus call does not provide a notification ID.
}

void InterfaceNotifyDBus::sendKeyShiftNotification(int keyshift)
{
    NotifyData data;
    data.id = this->keyshift_last_id;     // try to update existing one
    data.title = "KeyShift";
    data.body = QString::number(keyshift);
    data.timeout = -1;
    
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "Notify"
        );
    
    msg << "vkeybd-qt"
        << data.id
        << data.icon
        << data.title
        << data.body
        << data.actions
        << data.hints
        << data.timeout;
    
    notifyAsync(msg, &this->keyshift_last_id);
}

void InterfaceNotifyDBus::sendTabChangeNotification(const QString &text)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "Notify"
        );
    
    msg << "vkeybd-qt"
        << this->tabchange_last_id
        << ""                  // icon
        << "Register Change"        // title
        << text                // body
        << QStringList()
        << QVariantMap()
        << int(-1);          // timeout
    
    notifyAsync(msg, &this->tabchange_last_id);
}

void InterfaceNotifyDBus::notifyAsync(const QDBusMessage &msg, uint32_t *last_id)
{
    // not blocking: this runs on every tab switch, also while playing
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [last_id](QDBusPendingCallWatcher *finished_watcher){
        QDBusPendingReply<uint> reply = *finished_watcher;
        if (!reply.isError())
        {
            *last_id = reply.value();
        }
        finished_watcher->deleteLater();
    });
}


void InterfaceNotifyDBus::onNotificationClosed(uint id, uint reason)
{
    if (id == this->keyshift_last_id)
        this->keyshift_last_id = 0;
    
    if (id == this->tabchange_last_id)
        this->tabchange_last_id = 0;
}


