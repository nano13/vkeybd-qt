#ifndef INTERFACE_NOTIFY_DBUS_H
#define INTERFACE_NOTIFY_DBUS_H

#include <QObject>
#include <QString>
#include "enums_structs.h"

class InterfaceNotifyDBus : public QObject
{
    Q_OBJECT
    
public:
    explicit InterfaceNotifyDBus(QObject *parent = nullptr);
    
    static void sendNotification(const QString &message);
    int sendNotification(const NotificationData &data);
    
    void sendKeyShiftNotification(int keyshift);
    void sendTabChangeNotification(const QString &text);
    
private slots:
    void onNotificationClosed(uint id, uint reason);
    
private:
    uint32_t keyshift_last_id = 0;
    uint32_t tabchange_last_id = 0;
};

#endif // INTERFACE_NOTIFY_DBUS_H
