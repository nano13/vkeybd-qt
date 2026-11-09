#ifndef INPUTKEYBOARDRAW_H
#define INPUTKEYBOARDRAW_H

#include <QList>
#include <QObject>
#include <QSet>
#include <QString>

#include <memory>

#include "enums_structs.h"

/*
 * Raw keyboard input: key events of ONE physical keyboard, independent of
 * the window focus, so several keyboards can play different keyboard
 * sections. All platforms report Linux input key codes (KEY_* of
 * <linux/input.h>), like the focused-window input.
 *
 *   Linux    evdev (/dev/input/event*), lock = EVIOCGRAB
 *   Windows  Interception driver if installed (listen + lock),
 *            otherwise Raw Input (WM_INPUT, listen only)
 *   macOS    IOKit HID Manager, listen only; needs the Input Monitoring permission
 *
 * A lock that is not possible is an error (deviceNotAvailable), never a
 * silent fallback to listening: on stage, other applications must not get
 * the keys of a locked keyboard.
 *
 * The platform part lives in input_keyboard_raw_<platform>.cpp.
 */

// why a keyboard could not be opened or locked; the GUI explains each one
enum class RawKeyboardProblem
{
    Unavailable,            // generic, see the message
    Disconnected,           // unplugged or gone
    PermissionDenied,       // Linux: no read access to /dev/input/event*
    LockBusy,               // Linux: another program has grabbed the keyboard
    LockNeedsInterception,  // Windows: interception.dll missing
    InterceptionNotRunning, // Windows: interception.dll found, driver not running (reboot pending?)
    LockUnsupported,        // macOS: locking is not possible
    InputMonitoring,        // macOS: Input Monitoring permission missing
};

// one opened keyboard of a platform backend
class InputKeyboardRawBackend
{
public:
    virtual ~InputKeyboardRawBackend() = default;
};

class InputKeyboardRawMeta : public QObject
{
    Q_OBJECT
public:
    explicit InputKeyboardRawMeta(QObject *parent = nullptr);
    
    // display names ("name\n@id"), as shown in the combo box
    QList<QString> getKeyboardNames();
    // device id for InputKeyboardRawController; empty if unknown
    QString getPathForName(QString name);
    
    // whether keyboardLock() can keep the keys away from other applications
    // (Windows: only with the Interception driver)
    static bool canLock();
    
    struct Keyboard
    {
        QString name;
        QString id;
    };
    
private:
    // platform part
    static QList<Keyboard> detectKeyboards();
};

class InputKeyboardRawController : public QObject
{
    Q_OBJECT
public:
    explicit InputKeyboardRawController(QObject *parent = nullptr);
    ~InputKeyboardRawController() override;
    
    void keyboardListen(QString devpath);
    // fails with deviceNotAvailable() if the keyboard cannot be locked
    void keyboardLock(QString devpath);
    // stops listening; held keys are released first
    void keyboardRelease();
    
    // used by the platform backends (GUI thread)
    void reportKey(int keycode, bool pressed);
    void keyboardDisconnected();
    
signals:
    void deviceNotAvailable(QString message, RawKeyboardProblem problem);
    void signalRawKeyPressed(int keycode);
    void signalRawKeyReleased(int keycode);
    
private:
    std::unique_ptr<InputKeyboardRawBackend> backend;
    QSet<int> held_keys;
    
    void keyboardOpen(QString devpath, KeyboardMode mode);
};

// platform part: opens the keyboard with the given id, or returns nullptr and sets error and problem
std::unique_ptr<InputKeyboardRawBackend> createRawKeyboardBackend(InputKeyboardRawController *owner, const QString &id, KeyboardMode mode, QString &error, RawKeyboardProblem &problem);

#endif // INPUTKEYBOARDRAW_H
