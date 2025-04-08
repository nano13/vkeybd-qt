#include "input_keyboard_raw.h"

InputKeyboardRawMeta::InputKeyboardRawMeta(QObject *parent)
    : QObject{parent}
{
    
}

QList<QMap<QString,QString>> InputKeyboardRawMeta::detectKeyboards()
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
                QString ev = device["EV"];
                int evi = ev.toUInt(nullptr, 16);
                // https://unix.stackexchange.com/questions/74903/explain-ev-in-proc-bus-input-devices-data
                if ((evi & 0x120013) == 0x120013)
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
                                device["dev"] = hand.at(j);
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
QList<QString> InputKeyboardRawMeta::getKeyboardNames()
{
    QList<QMap<QString,QString>> keyboards = detectKeyboards();
    
    QList<QString> result;
    for (int i=0; i < keyboards.length(); i++)
    {
        QString keyboard_name = getKeyboardName(keyboards.at(i));
        //result.append(keyboards.at(i)["Name"]);
        result.append(keyboard_name);
    }
    
    return result;
}
QString InputKeyboardRawMeta::getPathForName(QString name)
{
    QList<QMap<QString,QString>> keyboards = detectKeyboards();
    
    for (int i=0; i < keyboards.length(); i++)
    {
        QString keyboard_name = getKeyboardName(keyboards.at(i));
        if (keyboard_name == name)
        {
            return keyboards.at(i)["devpath"];
        }
    }
    
    return "";
}
QString InputKeyboardRawMeta::getKeyboardName(QMap<QString,QString> keyboard)
{
    if (keyboard["Uniq"].length() > 0)
    {
        return keyboard["Name"] + "@" + keyboard["Uniq"];
    }
    else
    {
        return keyboard["Name"] + "@" + keyboard["dev"];
    }
}





InputKeyboardRawController::InputKeyboardRawController(QObject *parent)
    : QObject{parent}
{
    qDebug() << "0000000000000000000000000000000000000000000000000";
}

InputKeyboardRawController::~InputKeyboardRawController()
{
    keyboardRelease();
    
    //QMetaObject::invokeMethod(this->worker, "finish", Qt::QueuedConnection);

    //this->worker->finish();
    
    if (this->thread != nullptr)
    {
        //disconnect(this->thread);
        this->thread->terminate();
        this->thread->quit();
    }
    //this->thread->exit();
    //this->thread->deleteLater();
    
    //this->thread->requestInterruption();
    //this->thread->wait();

    //this->thread->deleteLater();
    //this->thread->wait();
}

void InputKeyboardRawController::keyboardListen(QString devpath)
{
    qDebug() << "listen";
    keyboardHelper(devpath, KeyboardMode::listen);
}
void InputKeyboardRawController::keyboardLock(QString devpath)
{
    qDebug() << "lock";
    keyboardHelper(devpath, KeyboardMode::lock);
}
void InputKeyboardRawController::keyboardHelper(QString devpath, KeyboardMode mode)
{
    // https://stackoverflow.com/questions/29942421/read-barcodes-from-input-event-linux-c/29956584#29956584
    // https://www.reddit.com/r/Cplusplus/comments/rsgjwf/ioctl_in_c_c_wrapper_class_for_linuxjoystickh/
    
    char *dev = devpath.toLocal8Bit().data();
    if ((this->fd = open(dev, O_RDONLY)) >= 0)
    {
        errno = 0;
        // to consume the event and not let it passed through to any other software
        if (mode == KeyboardMode::lock)
        {
            if (ioctl(this->fd, EVIOCGRAB, 1)) {
                //const int saved_errno = errno;
                //close(this->fd);
                //return errno = (saved_errno) ? errno : EACCES;
            }
        }
        
        this->worker = new InputKeyboardRawWorker(this->n, this->fd);
        //this->worker->initialize();
        connect(this->worker, &InputKeyboardRawWorker::rawKeyPressed, this, &InputKeyboardRawController::rawKeyPressed);
        connect(this->worker, &InputKeyboardRawWorker::rawKeyReleased, this, &InputKeyboardRawController::rawKeyReleased);
        
        this->thread = new QThread(this);
        this->worker->moveToThread(this->thread);
        connect(this->worker, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
        connect(this->thread, &QThread::started, this->worker, &InputKeyboardRawWorker::initialize);
        connect(this->worker, &InputKeyboardRawWorker::finished, this->thread, &QThread::quit);
        connect(this->worker, &InputKeyboardRawWorker::finished, this->worker, &InputKeyboardRawWorker::deleteLater);
        connect(this->thread, &QThread::finished, this->thread, &InputKeyboardRawWorker::deleteLater);
        /*
        connect(this->worker, &QObject::destroyed, this->thread, &QThread::quit);
        connect(this->worker, &InputKeyboardRawWorker::finished, this->worker, &QObject::deleteLater);
        connect(this->thread, &QThread::finished, this->thread, &QObject::deleteLater);
        */
        
        this->thread->start();
    }
    else
    {
        qDebug() << "ERROR: device not available";
        emit deviceNotAvailable("evdev open");
    }
}

void InputKeyboardRawController::keyboardRelease()
{
    qDebug() << "release: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAaaa";
    if (this->thread != nullptr)
    {
        //disconnect(this->thread);
        //this->thread->quit();
        //this->thread->exit();
        //this->thread->deleteLater();
        
        //this->thread->requestInterruption();
        //this->thread->wait();
        
        //this->thread->deleteLater();
    }
    
    //qDebug() << "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBbbb";
    if (this->fd != -1)
    {
        close(this->fd);
        this->fd = -1;
    }
}

void InputKeyboardRawController::rawKeyPressed(int keycode)
{
    qDebug() << keycode;
    emit signalRawKeyPressed(keycode);
}
void InputKeyboardRawController::rawKeyReleased(int keycode)
{
    emit signalRawKeyReleased(keycode);
}





InputKeyboardRawWorker::InputKeyboardRawWorker(ssize_t n, int fd, QObject *parent)
    : QObject{parent}
{
    this->n = n;
    this->fd = fd;
}

void InputKeyboardRawWorker::initialize()
{
    if (this->timer == nullptr)
    {
        this->timer = new QTimer(this);
        this->timer->setInterval(0);
        this->timer->setTimerType(Qt::PreciseTimer);
        connect(this->timer, &QTimer::timeout, this, &InputKeyboardRawWorker::tick, Qt::DirectConnection);
        
        this->timer->start();
    }
}
void InputKeyboardRawWorker::timerStart()
{
    this->timer->start();
}
void InputKeyboardRawWorker::timerStop()
{
    this->timer->stop();
}
/*
InputKeyboardRawWorker::~InputKeyboardRawWorker()
{
    qDebug() << "RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR";
    this->timer->disconnect();
    disconnect(this->timer);
    this->timer->stop();
}
*/
void InputKeyboardRawWorker::tick()
{
    qDebug() << "keyboard raw ticking";
    // https://stackoverflow.com/questions/20943322/accessing-keys-from-linux-input-device
    
    this->n = read(this->fd, &this->ev, sizeof this->ev);
    qDebug() << "kkkk";
    
    if (this->n != (ssize_t)-1 && this->n == sizeof this->ev)
    {
        if (this->ev.type == EV_KEY && this->ev.value >= 0 && this->ev.value <= 2)
        {
            //printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
            if (this->ev.value == 1)
            {
                qDebug() << "KEY PRESSED RAW";
                emit rawKeyPressed(this->ev.code);
            }
            else if (this->ev.value == 0)
            {
                qDebug() << "KEY RELEASED RAW";
                emit rawKeyReleased(this->ev.code);
            }
        }
    }
}

void InputKeyboardRawWorker::finish()
{
    //this->timer->disconnect();
    //disconnect(this->timer);
    /*
    if (this->timer)
    {
        if (this->timer->isActive())
        {
            this->timer->stop();
        }
        
        //delete this->timer;
        this->timer = nullptr;
    }
    */
    //emit finished();
}
