#include "input_keyboard_qt.h"

InputKeyboardQt::InputKeyboardQt(QObject *parent)
    : QObject{parent}
{
    
}

bool InputKeyboardQt::callEventFilter(QObject *obj, QEvent *ev)
{
    Q_UNUSED(obj);

    if (ev->type() != QEvent::KeyPress &&
        ev->type() != QEvent::KeyRelease &&
        ev->type() != QEvent::ShortcutOverride)
        return false;

    QKeyEvent *event = static_cast<QKeyEvent*>(ev);
    const quint32 scan_code = event->nativeScanCode();

    // Synthetic Qt key events have no X11 scan code; subtracting eight would
    // otherwise invent a negative Linux key code.
    if (scan_code < 8)
        return false;

    const int key = static_cast<int>(scan_code - 8);

    if (ev->type() == QEvent::ShortcutOverride)
    {
        // Prevent focused child widgets and QAction shortcuts from stealing the
        // subsequent KeyPress, but do not treat ShortcutOverride as a press.
        event->accept();
        return true;
    }

    if (ev->type() == QEvent::KeyPress)
    {
        // Both native and Qt-generated repeat presses are ignored while held.
        if (!this->held_keys.contains(key))
        {
            this->held_keys.insert(key);
            emit keyPressSignal(key);
        }
    }
    else
    {
        // Ignore intermediary auto-repeat releases. The final physical release
        // clears the held key even if its Qt key symbol changed in the meantime.
        if (!event->isAutoRepeat() && this->held_keys.remove(key))
            emit keyReleaseSignal(key);
    }

    return true;
}

void InputKeyboardQt::releaseAllKeys()
{
    const QList<int> keys = this->held_keys.values();
    this->held_keys.clear();
    for (int key : keys)
        emit keyReleaseSignal(key);
}

int InputKeyboardQt::mapFromKeyboardLayoutToLinuxRaw(int key)
{
    QMap<int, int> map_layout;
    
    QList<int> list_qt;
    list_qt = {Qt::Key_F1, Qt::Key_F2, Qt::Key_F3, Qt::Key_F4,
               Qt::Key_F5, Qt::Key_F6, Qt::Key_F7, Qt::Key_F8,
               Qt::Key_F9, Qt::Key_F10, Qt::Key_F11, Qt::Key_F12,
              Qt::Key_Up, Qt::Key_Down, Qt::Key_PageUp, Qt::Key_PageDown, Qt::Key_Left, Qt::Key_Right, Qt::Key_Escape, Qt::Key_Delete, Qt::Key_Insert, Qt::Key_Menu, Qt::Key_Space, Qt::Key_Alt, Qt::Key_Super_L, Qt::Key_Super_R,
               16781906, 49, 50, 51, 52, 53, 54, 55, 56, 57, 48, 45, 16781904, 16777219,
               16777217, 88, 86, 76, 67, 87, 75, 72, 71, 70, 81, 223, 16781905, 16777220,
               16781571, 85, 73, 65, 69, 79, 83, 78, 82, 84, 68, 89, 16781571,
               16777252, 0, 220, 214, 196, 80, 90, 66, 77, 44, 46, 74, 16777248,
               -1, 16777299, 16777251, 32, 16777251, -1, -1, -1, -1, -1, -1};
    
    QList<int> list_raw;
    list_raw = {KeysRaw::F1, KeysRaw::F2, KeysRaw::F3, KeysRaw::F4,
                KeysRaw::F5, KeysRaw::F6, KeysRaw::F7, KeysRaw::F8,
                KeysRaw::F9, KeysRaw::F10, KeysRaw::F11, KeysRaw::F12,
               KeysRaw::Up, KeysRaw::Down, KeysRaw::PageUp, KeysRaw::PageDown, KeysRaw::Left, KeysRaw::Right, KeysRaw::Escape, KeysRaw::Delete, KeysRaw::Insert, KeysRaw::Menu, KeysRaw::Space, KeysRaw::Alt, KeysRaw::Super_L, KeysRaw::Super_R,
                41, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
                58, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 43,
                42, 86, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
                -1, 125, 56, 57, 100, -1, -1, -1, -1, -1, -1};
    
    if (list_qt.length() == list_raw.length())
    {
        for (int i=0; i < list_raw.length(); i++)
        {
            map_layout[list_qt.at(i)] = list_raw.at(i);
        }
    }
    
    return map_layout[key];
}

void InputKeyboardQt::sendUDPMessage(QString message)
{
    
}
