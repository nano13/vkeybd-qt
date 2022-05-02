#include "input_keyboard_raw.h"

InputKeyboardRaw::InputKeyboardRaw(QObject *parent)
    : QObject{parent}
{
    
}

QList<QMap<QString,QString>> InputKeyboardRaw::detectKeyboards()
{
    QList<QMap<QString,QString>> keyboards;
    
    QFile file("/proc/bus/input/devices");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&file);
        QString devs = stream.readAll();
        QList<QString> lines = devs.split("\n");
        
        QMap<QString,QString> device;
        for (int i=0; i < lines.length(); i++)
        {
            QString line = lines.at(i);
            
            if (line.isEmpty())
            {
                // https://unix.stackexchange.com/questions/74903/explain-ev-in-proc-bus-input-devices-data
                if (device["EV"] == "120013")
                {
                    keyboards.append(device);
                }
                device.clear();
            }
            else
            {
                QList<QString> splitted = line.split(": ").at(1).split("=");
                if (splitted.length() > 1)
                {
                    if (splitted.at(0) == "Handlers")
                    {
                        QList<QString> hand = splitted.at(1).split(" ");
                        for (int j=0; j < hand.length(); j++)
                        {
                            if (hand.at(j).startsWith("event"))
                            {
                                device["devpath"] = "/dev/input/"+hand.at(j);
                            }
                        }
                    }
                    QString key = splitted.at(0);
                    QString value = splitted.at(1);
                    device[key] = value.remove("\"");
                }
            }
        }
    }
    file.close();
    
    return keyboards;
}
QList<QString> InputKeyboardRaw::getKeyboardNames()
{
    QList<QMap<QString,QString>> keyboards = detectKeyboards();
    
    QList<QString> result;
    for (int i=0; i < keyboards.length(); i++)
    {
        result.append(keyboards.at(i)["Name"]);
    }
    
    return result;
}
QString InputKeyboardRaw::getPathForName(QString name)
{
    QList<QMap<QString,QString>> keyboards = detectKeyboards();
    
    for (int i=0; i < keyboards.length(); i++)
    {
        if (keyboards.at(i)["Name"] == name)
        {
            return keyboards.at(i)["devpath"];
        }
    }
    
    return "";
}

void InputKeyboardRaw::keyboardListen(QString devpath)
{
    /*
    QFile file(devpath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Binary))
    {
        QDataStream stream(&file);
        qDebug() << stream.readRawData();
    }
    */
}
void InputKeyboardRaw::keyboardLock(QString devpath)
{
    qDebug() << "lockraiaeuie";
    // https://stackoverflow.com/questions/29942421/read-barcodes-from-input-event-linux-c/29956584#29956584
    // https://www.reddit.com/r/Cplusplus/comments/rsgjwf/ioctl_in_c_c_wrapper_class_for_linuxjoystickh/
    
    char *dev = devpath.toLocal8Bit().data();
    if ((this->fd = open(dev, O_RDONLY)) >= 0)
    {
        errno = 0;
        // to consume the event and not let it passed through to any other software
        if (ioctl(this->fd, EVIOCGRAB, 1)) {
            const int saved_errno = errno;
            close(this->fd);
            //return errno = (saved_errno) ? errno : EACCES;
        }
        
        this->worker = new InputKeyboardRawWorker(this->n, this->fd);
        connect(this->worker, &InputKeyboardRawWorker::rawKeyPressed, this, &InputKeyboardRaw::rawKeyPressed);
        connect(this->worker, &InputKeyboardRawWorker::rawKeyReleased, this, &InputKeyboardRaw::rawKeyReleased);
        
        this->thread = new QThread(this);
        this->worker->moveToThread(this->thread);
        this->thread->start();
        
        // https://stackoverflow.com/questions/20943322/accessing-keys-from-linux-input-device
        /*
        while (true)
        {
            n = read(this->fd, &this->ev, sizeof this->ev);
            
            if (n == (ssize_t)-1)
            {
                continue;
            }
            else if (n != sizeof ev)
            {
                continue;
            }
            
            if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2)
            {
                //printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
                if (ev.value == 1)
                {
                    emit rawKeyPressed(ev.code);
                }
                else if (ev.value == 0)
                {
                    emit rawKeyReleased(ev.code);
                }
            }
        }
        */
    }
    else
    {
        emit deviceNotAvailable("evdev open");
    }
}

void InputKeyboardRaw::keyboardRelease()
{
    this->thread->exit();
    close(this->fd);
}

void InputKeyboardRaw::rawKeyPressed(int keycode)
{
    emit rawKeyPressedSignal(keycode);
}
void InputKeyboardRaw::rawKeyReleased(int keycode)
{
    emit rawKeyReleasedSignal(keycode);
}



InputKeyboardRawWorker::InputKeyboardRawWorker(ssize_t n, int fd, QObject *parent)
    : QObject{parent}
{
    this->n = n;
    this->fd = fd;
    
    this->timer = new QTimer(this);
    this->timer->setInterval(1);
    this->timer->setTimerType(Qt::PreciseTimer);
    connect(this->timer, &QTimer::timeout, this, &InputKeyboardRawWorker::tick, Qt::DirectConnection);
    
    this->timer->start();
}

void InputKeyboardRawWorker::tick()
{
    this->n = read(this->fd, &this->ev, sizeof this->ev);
    
    if (this->n != (ssize_t)-1 && this->n == sizeof this->ev)
    {
        if (this->ev.type == EV_KEY && this->ev.value >= 0 && this->ev.value <= 2)
        {
            //printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
            if (this->ev.value == 1)
            {
                emit rawKeyPressed(this->ev.code);
            }
            else if (this->ev.value == 0)
            {
                emit rawKeyReleased(this->ev.code);
            }
        }
    }
}