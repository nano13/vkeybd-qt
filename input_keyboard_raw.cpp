#include "input_keyboard_raw.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

// ---------------- Worker ----------------
InputKeyboardRawWorker::InputKeyboardRawWorker(int fd, QObject *parent)
    : QObject(parent), fd(fd)
{
}

InputKeyboardRawWorker::~InputKeyboardRawWorker()
{
    stop();
}

void InputKeyboardRawWorker::initialize()
{
    if (!notifier)
    {
        notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        connect(notifier, &QSocketNotifier::activated,
                this, &InputKeyboardRawWorker::readEvent, Qt::DirectConnection);
    }
}

void InputKeyboardRawWorker::readEvent(int)
{
    struct input_event ev;
    ssize_t n = read(fd, &ev, sizeof(ev));
    
    if (n == sizeof(ev))
    {
        if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2)
        {
            if (ev.value == 1)
                emit rawKeyPressed(ev.code);
            else if (ev.value == 0)
                emit rawKeyReleased(ev.code);
        }
    }
    else
    {
        qWarning() << "Keyboard read error or disconnected:" << strerror(errno);
        emit deviceDisconnected();
    }
}

void InputKeyboardRawWorker::stop()
{
    if (notifier) {
        notifier->setEnabled(false);
        notifier->deleteLater();
        notifier = nullptr;
    }
    
    if (fd != -1) {
        ioctl(fd, EVIOCGRAB, 0);  // IMPORTANT
        ::close(fd);
        fd = -1;
    }
}








// ---------------- Meta ----------------
InputKeyboardRawMeta::InputKeyboardRawMeta(QObject *parent)
    : QObject(parent)
{
}

QList<QMap<QString, QString>> InputKeyboardRawMeta::detectKeyboards()
{
    QList<QMap<QString, QString>> keyboards;
    
    QFile file("/proc/bus/input/devices");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return keyboards;
    
    QTextStream stream(&file);
    QString devs = stream.readAll();
    QList<QString> lines = devs.split("\n");
    
    QMap<QString, QString> device;
    
    for (const QString &line : lines)
    {
        if (line.isEmpty())
        {
            QString ev = device["EV"];
            int evi = ev.toUInt(nullptr, 16);
            if ((evi & 0x120013) == 0x120013)
                keyboards.append(device);
            device.clear();
        }
        else
        {
            if (line.contains(": "))
            {
                QList<QString> splitted = line.split(": ").at(1).split("=");
                if (splitted.length() > 1)
                {
                    if (splitted.at(0) == "Handlers")
                    {
                        for (const QString &hand : splitted.at(1).split(" "))
                        {
                            if (hand.startsWith("event"))
                            {
                                device["devpath"] = "/dev/input/" + hand;
                                device["dev"] = hand;
                            }
                        }
                    }
                    
                    // FIX: create a copy so remove() can be called
                    QString key = splitted.at(0);
                    QString value = splitted.at(1);
                    value.remove("\"");  // safe now
                    device[key] = value;
                }
            }
        }
    }
    
    return keyboards;
}


QList<QString> InputKeyboardRawMeta::getKeyboardNames()
{
    QList<QMap<QString, QString>> keyboards = detectKeyboards();
    QList<QString> result;
    for (auto &k : keyboards)
        result.append(getKeyboardName(k));
    return result;
}

QString InputKeyboardRawMeta::getPathForName(QString name)
{
    for (auto &k : detectKeyboards())
    {
        if (getKeyboardName(k) == name)
            return k["devpath"];
    }
    return "";
}

QString InputKeyboardRawMeta::getKeyboardName(QMap<QString, QString> keyboard)
{
    return keyboard["Name"] + "@" + (keyboard["Uniq"].isEmpty() ? keyboard["dev"] : keyboard["Uniq"]);
}





// ---------------- Controller ----------------
InputKeyboardRawController::InputKeyboardRawController(QObject *parent)
    : QObject(parent)
{
}

InputKeyboardRawController::~InputKeyboardRawController()
{
    keyboardRelease();
}

void InputKeyboardRawController::keyboardListen(QString devpath)
{
    keyboardHelper(devpath, KeyboardMode::listen);
}

void InputKeyboardRawController::keyboardLock(QString devpath)
{
    keyboardHelper(devpath, KeyboardMode::lock);
}

void InputKeyboardRawController::keyboardHelper(QString devpath, KeyboardMode mode)
{
    keyboardRelease(); // release previous keyboard safely
    
    QByteArray devBytes = devpath.toLocal8Bit();
    const char* dev = devBytes.constData();
    
    int newFd = open(dev, O_RDONLY);
    if (newFd < 0)
    {
        qDebug() << "ERROR: device not available:" << strerror(errno);
        emit deviceNotAvailable("evdev open");
        return;
    }
    
    if (mode == KeyboardMode::lock)
    {
        if (ioctl(newFd, EVIOCGRAB, 1))
            qWarning() << "Failed to grab keyboard:" << strerror(errno);
    }
    
    worker = new InputKeyboardRawWorker(newFd);
    thread = new QThread(this);
    
    worker->moveToThread(thread);
    
    connect(worker, &InputKeyboardRawWorker::rawKeyPressed,
            this, &InputKeyboardRawController::rawKeyPressed);
    connect(worker, &InputKeyboardRawWorker::rawKeyReleased,
            this, &InputKeyboardRawController::rawKeyReleased);
    
    connect(worker, &InputKeyboardRawWorker::deviceDisconnected,
            this, &InputKeyboardRawController::keyboardDisconnected);
    
    connect(thread, &QThread::started, worker, &InputKeyboardRawWorker::initialize);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    
    thread->start();
}

void InputKeyboardRawController::keyboardDisconnected()
{
    qDebug() << "Keyboard disconnected";
    emit deviceNotAvailable("Keyboard disconnected");
    keyboardRelease();
}
void InputKeyboardRawController::keyboardRelease()
{
    if (!worker)
        return;
    
    InputKeyboardRawWorker *w = worker;
    QThread *t = thread;
    
    worker = nullptr;
    thread = nullptr;
    
    // Stop notifier + fd inside worker thread, synchronously
    QMetaObject::invokeMethod(
        w,
        &InputKeyboardRawWorker::stop,
        Qt::BlockingQueuedConnection
        );
    
    t->quit();
    t->wait();
    
    w->deleteLater();
    t->deleteLater();
}


void InputKeyboardRawController::rawKeyPressed(int keycode)
{
    emit signalRawKeyPressed(keycode);
}

void InputKeyboardRawController::rawKeyReleased(int keycode)
{
    emit signalRawKeyReleased(keycode);
}
