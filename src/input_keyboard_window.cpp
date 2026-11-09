#include "input_keyboard_window.h"

#include <QKeyEvent>

InputKeyboardWindow::InputKeyboardWindow(QObject *parent)
    : QObject{parent}
{
    
}

int InputKeyboardWindow::linuxKeyCode(const QKeyEvent *event)
{
    // X11 and Wayland report XKB key codes, which are the Linux key codes + 8.
    // Synthetic Qt key events have no scan code.
    // Porting: Windows (PS/2 set 1 scan codes, E0-extended keys differ) and
    // macOS (nativeVirtualKey) need their own translation here.
    const quint32 scan_code = event->nativeScanCode();
    if (scan_code < 8)
        return -1;
    return static_cast<int>(scan_code - 8);
}

bool InputKeyboardWindow::handleKeyEvent(QEvent *ev)
{
    if (ev->type() != QEvent::KeyPress &&
        ev->type() != QEvent::KeyRelease &&
        ev->type() != QEvent::ShortcutOverride)
        return false;
    
    QKeyEvent *event = static_cast<QKeyEvent*>(ev);
    const int key = linuxKeyCode(event);
    if (key < 0)
        return false;
    
    if (ev->type() == QEvent::ShortcutOverride)
    {
        // keep focused widgets and QAction shortcuts from stealing the
        // following KeyPress, but do not treat it as a press
        event->accept();
        return true;
    }
    
    if (ev->type() == QEvent::KeyPress)
    {
        // auto-repeat presses are ignored while the key is held
        if (!this->held_keys.contains(key))
        {
            this->held_keys.insert(key);
            emit keyPressSignal(key);
        }
    }
    else
    {
        // auto-repeat releases are ignored; the physical release clears the
        // key even if its Qt key symbol changed in the meantime
        if (!event->isAutoRepeat() && this->held_keys.remove(key))
            emit keyReleaseSignal(key);
    }
    
    return true;
}

void InputKeyboardWindow::releaseAllKeys()
{
    const QList<int> keys = this->held_keys.values();
    this->held_keys.clear();
    for (const int key : keys)
        emit keyReleaseSignal(key);
}
