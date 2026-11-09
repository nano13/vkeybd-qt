#ifndef INPUTKEYBOARDWINDOW_H
#define INPUTKEYBOARDWINDOW_H

#include <QObject>
#include <QSet>

class QEvent;
class QKeyEvent;

/*
 * Keyboard input "Focused window": key events that Qt delivers to the
 * vkeybd-qt window, reported as Linux input key codes (KEY_* of
 * <linux/input.h>, the same codes as the raw evdev input), so the keyboard
 * layout and modifiers do not matter.
 */
class InputKeyboardWindow : public QObject
{
    Q_OBJECT
public:
    explicit InputKeyboardWindow(QObject *parent = nullptr);
    
    // returns true if the event was consumed
    bool handleKeyEvent(QEvent *ev);
    // emits a release for every held key (e.g. when the window loses the focus)
    void releaseAllKeys();
    
private:
    // physical keys instead of Qt key symbols: Shift or layout changes must
    // not prevent a release from matching its press
    QSet<int> held_keys;
    
    // Linux key code of the physical key, or -1
    static int linuxKeyCode(const QKeyEvent *event);
    
signals:
    void keyPressSignal(int key);
    void keyReleaseSignal(int key);
};

#endif // INPUTKEYBOARDWINDOW_H
