#ifndef INTERFACE_NOTIFY_H
#define INTERFACE_NOTIFY_H

#include <QObject>
#include <QString>

#include "enums_structs.h"

class InterfaceNotify : public QObject
{
public:
    explicit InterfaceNotify(QObject *parent = nullptr);
    
    static void sendNotification(const QString &message);
    virtual int sendNotification(const NotifyData &data);
    
    virtual void sendKeyShiftNotification(int keyshift);
    virtual void sendTabChangeNotification(const QString &text);
    
private slots:
    
private:
    uint32_t keyshift_last_id = 0;
    uint32_t tabchange_last_id = 0;
};

#endif // INTERFACE_NOTIFY_H
