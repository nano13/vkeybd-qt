// Raw keyboard input on Linux: evdev (/dev/input/event*)

#include "input_keyboard_raw.h"

#include <QDebug>
#include <QFile>
#include <QMap>
#include <QSocketNotifier>
#include <QTextStream>

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace
{
class EvdevBackend : public InputKeyboardRawBackend
{
public:
    EvdevBackend(InputKeyboardRawController *owner, int fd, bool grabbed)
        : owner(owner), fd(fd), grabbed(grabbed)
    {
        // GUI thread: the MIDI output runs there anyway
        this->notifier = std::make_unique<QSocketNotifier>(fd, QSocketNotifier::Read);
        QObject::connect(this->notifier.get(), &QSocketNotifier::activated, owner, [this]() { readEvents(); });
    }
    
    ~EvdevBackend() override
    {
        this->notifier.reset();
        if (this->grabbed)
            ioctl(this->fd, EVIOCGRAB, 0);
        ::close(this->fd);
    }
    
private:
    InputKeyboardRawController *owner;
    int fd;
    bool grabbed;
    std::unique_ptr<QSocketNotifier> notifier;
    
    void readEvents()
    {
        struct input_event events[64];
        const ssize_t bytes = ::read(this->fd, events, sizeof(events));
        if (bytes < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
                return;
            
            qWarning() << "Keyboard read error or disconnected:" << strerror(errno);
            this->notifier->setEnabled(false);
            // not from inside the notifier: keyboardDisconnected() deletes this backend
            InputKeyboardRawController *controller = this->owner;
            QMetaObject::invokeMethod(controller, [controller]() { controller->keyboardDisconnected(); }, Qt::QueuedConnection);
            return;
        }
        
        const int count = static_cast<int>(bytes / static_cast<ssize_t>(sizeof(struct input_event)));
        for (int i = 0; i < count; i++)
        {
            // value: 0 release, 1 press, 2 auto-repeat (ignored)
            if (events[i].type == EV_KEY && (events[i].value == 0 || events[i].value == 1))
                this->owner->reportKey(events[i].code, events[i].value == 1);
        }
    }
};
}

bool InputKeyboardRawMeta::canLock()
{
    return true;
}

QList<InputKeyboardRawMeta::Keyboard> InputKeyboardRawMeta::detectKeyboards()
{
    QList<Keyboard> keyboards;
    
    QFile file("/proc/bus/input/devices");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return keyboards;
    
    QTextStream stream(&file);
    const QList<QString> lines = stream.readAll().split("\n");
    
    // one block per device, separated by empty lines
    QMap<QString, QString> device;
    for (const QString &line : lines)
    {
        if (line.isEmpty())
        {
            // EV_SYN | EV_KEY | EV_MSC | EV_LED | EV_REP: a real keyboard
            const uint ev = device.value("EV").toUInt(nullptr, 16);
            if ((ev & 0x120013) == 0x120013 && device.contains("devpath"))
            {
                Keyboard keyboard;
                keyboard.name = device.value("Name") + "\n@" + (device.value("Uniq").isEmpty() ? device.value("dev") : device.value("Uniq"));
                keyboard.id = device.value("devpath");
                keyboards.append(keyboard);
            }
            device.clear();
            continue;
        }
        
        if (!line.contains(": "))
            continue;
        
        const QList<QString> splitted = line.split(": ").at(1).split("=");
        if (splitted.length() < 2)
            continue;
        
        if (splitted.at(0) == "Handlers")
        {
            for (const QString &handler : splitted.at(1).split(" "))
            {
                if (handler.startsWith("event"))
                {
                    device["devpath"] = "/dev/input/" + handler;
                    device["dev"] = handler;
                }
            }
        }
        
        QString value = splitted.at(1);
        value.remove("\"");
        device[splitted.at(0)] = value;
    }
    
    return keyboards;
}

std::unique_ptr<InputKeyboardRawBackend> createRawKeyboardBackend(InputKeyboardRawController *owner, const QString &id, KeyboardMode mode, QString &error, RawKeyboardProblem &problem)
{
    const QByteArray path = id.toLocal8Bit();
    const int fd = ::open(path.constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
    {
        const int open_error = errno;
        error = QString("Cannot open ") + id + ": " + strerror(open_error);
        if (open_error == EACCES || open_error == EPERM)
            problem = RawKeyboardProblem::PermissionDenied;
        else if (open_error == ENOENT || open_error == ENODEV || open_error == ENXIO)
            problem = RawKeyboardProblem::Disconnected;
        return nullptr;
    }
    
    bool grabbed = false;
    if (mode == KeyboardMode::lock)
    {
        // no silent fallback to listening: other applications would get the keys
        if (ioctl(fd, EVIOCGRAB, 1) != 0)
        {
            const int grab_error = errno;
            ::close(fd);
            if (grab_error == EBUSY)
            {
                error = "Cannot lock the keyboard: another program has grabbed it already.";
                problem = RawKeyboardProblem::LockBusy;
            }
            else
            {
                error = QString("Cannot lock the keyboard: ") + strerror(grab_error);
            }
            return nullptr;
        }
        grabbed = true;
    }
    
    return std::make_unique<EvdevBackend>(owner, fd, grabbed);
}
