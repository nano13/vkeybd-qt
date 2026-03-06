#ifndef INTERFACE_NOTIFY_H
#define INTERFACE_NOTIFY_H

#include <QString>

#include "enums_structs.h"

class InterfaceNotify
{
public:
    virtual ~InterfaceNotify() = default;
    
    virtual void sendNotification(const QString &message) = 0;
    virtual int sendNotification(const NotificationData &data);
};

#endif // INTERFACE_NOTIFY_H
