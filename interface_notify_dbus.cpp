#include "interface_notify_dbus.h"
#include <QDBusMessage>
#include <QDBusConnection>

InterfaceNotifyDBus::InterfaceNotifyDBus(QObject *parent)
    : QObject(parent)
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

int InterfaceNotifyDBus::sendNotification(const NotificationData &data)
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
}

void InterfaceNotifyDBus::sendKeyShiftNotification(int keyshift)
{
    NotificationData data;
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
    
    // MUST be blocking to get the returned ID
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    
    if (reply.type() == QDBusMessage::ReplyMessage &&
        reply.arguments().size() == 1)
    {
        this->keyshift_last_id = reply.arguments().at(0).toUInt();
    }
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
    
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    
    if (reply.type() == QDBusMessage::ReplyMessage &&
        reply.arguments().size() == 1)
    {
        this->tabchange_last_id = reply.arguments().at(0).toUInt();
    }
}


void InterfaceNotifyDBus::onNotificationClosed(uint id, uint reason)
{
    if (id == this->keyshift_last_id)
        this->keyshift_last_id = 0;
    
    if (id == this->tabchange_last_id)
        this->tabchange_last_id = 0;
}


