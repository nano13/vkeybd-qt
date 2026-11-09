#include "input_keyboard_raw.h"

#include <QDebug>

// ---------------- Meta ----------------
InputKeyboardRawMeta::InputKeyboardRawMeta(QObject *parent)
    : QObject(parent)
{
}

QList<QString> InputKeyboardRawMeta::getKeyboardNames()
{
    QList<QString> names;
    const QList<Keyboard> keyboards = detectKeyboards();
    for (const Keyboard &keyboard : keyboards)
        names.append(keyboard.name);
    return names;
}

QString InputKeyboardRawMeta::getPathForName(QString name)
{
    const QList<Keyboard> keyboards = detectKeyboards();
    for (const Keyboard &keyboard : keyboards)
    {
        if (keyboard.name == name)
            return keyboard.id;
    }
    return QString();
}

// ---------------- Controller ----------------
InputKeyboardRawController::InputKeyboardRawController(QObject *parent)
    : QObject(parent)
{
}

InputKeyboardRawController::~InputKeyboardRawController()
{
    // no releases from a destructor: the receivers may be gone already
    this->held_keys.clear();
    this->backend.reset();
}

void InputKeyboardRawController::keyboardListen(QString devpath)
{
    keyboardOpen(devpath, KeyboardMode::listen);
}

void InputKeyboardRawController::keyboardLock(QString devpath)
{
    keyboardOpen(devpath, KeyboardMode::lock);
}

void InputKeyboardRawController::keyboardOpen(QString devpath, KeyboardMode mode)
{
    keyboardRelease();
    
    QString error;
    this->backend = createRawKeyboardBackend(this, devpath, mode, error);
    if (!this->backend)
    {
        qWarning() << "Keyboard not available:" << devpath << error;
        emit deviceNotAvailable(error);
    }
}

void InputKeyboardRawController::keyboardRelease()
{
    this->backend.reset();
    
    // a key held while switching would otherwise never be released
    const QList<int> keys = this->held_keys.values();
    this->held_keys.clear();
    for (const int key : keys)
        emit signalRawKeyReleased(key);
}

void InputKeyboardRawController::reportKey(int keycode, bool pressed)
{
    if (keycode < 0)
        return;
    
    // backends may report auto-repeat as further presses
    if (pressed)
    {
        if (this->held_keys.contains(keycode))
            return;
        this->held_keys.insert(keycode);
        emit signalRawKeyPressed(keycode);
    }
    else if (this->held_keys.remove(keycode))
    {
        emit signalRawKeyReleased(keycode);
    }
}

void InputKeyboardRawController::keyboardDisconnected()
{
    qDebug() << "Keyboard disconnected";
    keyboardRelease();
    emit deviceNotAvailable("Keyboard disconnected");
}
