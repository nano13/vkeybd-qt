// Raw keyboard input on Windows.
//
// With the Interception driver installed (https://github.com/oblitum/Interception,
// interception.dll next to the executable or on the PATH): every keyboard is
// read through the driver. "Listen" passes the keys on to Windows, "Lock"
// keeps them away from Windows and all other applications.
//
// Without the driver: Raw Input API (WM_INPUT), listening only. One
// process-wide registration for keyboards with RIDEV_INPUTSINK, so the keys
// arrive even without the window focus; the WM_INPUT messages go to a
// message-only window and are picked up with a native event filter.

#include "input_keyboard_raw.h"

#include <QAbstractNativeEventFilter>
#include <QCoreApplication>
#include <QDebug>
#include <QHash>

#include <array>
#include <atomic>
#include <string>
#include <thread>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <hidsdi.h>

namespace
{
class RawInputBackend;

class RawInputHub : public QAbstractNativeEventFilter
{
public:
    static RawInputHub &instance()
    {
        // never destroyed: the registration lives as long as the process
        static RawInputHub *hub = new RawInputHub;
        return *hub;
    }

    bool add(RawInputBackend *backend, QString &error);
    void remove(RawInputBackend *backend);

    QString deviceName(HANDLE device);

    bool nativeEventFilter(const QByteArray &event_type, void *message, qintptr *result) override;

private:
    HWND window = nullptr;
    bool registered = false;
    QList<RawInputBackend*> backends;
    // device handle -> device path; kept after removal to report the disconnect
    QHash<HANDLE, QString> device_names;

    bool ensureRegistered(QString &error);
    void handleInput(HRAWINPUT input);
    void handleDeviceChange(WPARAM change, HANDLE device);
};

class RawInputBackend : public InputKeyboardRawBackend
{
public:
    RawInputBackend(InputKeyboardRawController *owner, const QString &id)
        : owner(owner), id(id)
    {
    }

    ~RawInputBackend() override
    {
        RawInputHub::instance().remove(this);
    }

    InputKeyboardRawController *owner;
    QString id;
};

// Linux key code for a PS/2 set 1 scan code, or -1. The Linux key codes are
// based on set 1: keys without the E0 prefix map 1:1, the E0 keys need a table.
int scanCodeToLinux(unsigned int code, bool e0)
{
    if (e0)
    {
        switch (code)
        {
            case 0x1C: return 96;  // KEY_KPENTER
            case 0x1D: return 97;  // KEY_RIGHTCTRL
            case 0x35: return 98;  // KEY_KPSLASH
            case 0x37: return 99;  // KEY_SYSRQ (Print)
            case 0x38: return 100; // KEY_RIGHTALT
            case 0x47: return 102; // KEY_HOME
            case 0x48: return 103; // KEY_UP
            case 0x49: return 104; // KEY_PAGEUP
            case 0x4B: return 105; // KEY_LEFT
            case 0x4D: return 106; // KEY_RIGHT
            case 0x4F: return 107; // KEY_END
            case 0x50: return 108; // KEY_DOWN
            case 0x51: return 109; // KEY_PAGEDOWN
            case 0x52: return 110; // KEY_INSERT
            case 0x53: return 111; // KEY_DELETE
            case 0x5B: return 125; // KEY_LEFTMETA
            case 0x5C: return 126; // KEY_RIGHTMETA
            case 0x5D: return 127; // KEY_COMPOSE (Menu)
            default:   return -1;  // incl. the fake shifts E0 2A / E0 36
        }
    }
    
    if (code == 0 || code > 0x58)
        return -1;
    return static_cast<int>(code);
}

// Linux key code for a Raw Input keyboard event, or -1
int linuxKeyCode(const RAWKEYBOARD &keyboard)
{
    if (keyboard.MakeCode == KEYBOARD_OVERRUN_MAKE_CODE || keyboard.VKey == 0xFF)
        return -1; // overrun, or a fake key of an E0/E1 sequence
    if (keyboard.VKey == VK_PAUSE)
        return 119; // KEY_PAUSE (E1 1D 45)
    if (keyboard.Flags & RI_KEY_E1)
        return -1;
    return scanCodeToLinux(keyboard.MakeCode, (keyboard.Flags & RI_KEY_E0) != 0);
}

QString productName(const QString &path)
{
    // only HID keyboards have one; access 0 is enough for the attributes
    const std::wstring wide_path = path.toStdWString();
    HANDLE file = CreateFileW(wide_path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return QString();

    wchar_t product[127] = {};
    QString name;
    if (HidD_GetProductString(file, product, sizeof(product)))
        name = QString::fromWCharArray(product).trimmed();
    CloseHandle(file);
    return name;
}

// "\\?\HID#VID_046D&PID_C31C&MI_00#7&2a1b3c&0&0000#{guid}" -> "VID_046D&PID_C31C&MI_00#7&2a1b3c&0&0000"
QString shortId(const QString &path)
{
    const QList<QString> parts = path.split('#');
    if (parts.size() >= 3)
        return parts.at(1) + "#" + parts.at(2);
    return path;
}

bool RawInputHub::ensureRegistered(QString &error)
{
    if (this->registered)
        return true;

    HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!this->window)
    {
        WNDCLASSW window_class = {};
        window_class.lpfnWndProc = DefWindowProcW;
        window_class.hInstance = instance;
        window_class.lpszClassName = L"vkeybd-qt-raw-input";
        RegisterClassW(&window_class);

        this->window = CreateWindowExW(0, window_class.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, nullptr);
        if (!this->window)
        {
            error = "Raw Input: cannot create the message window";
            return false;
        }
    }

    RAWINPUTDEVICE device = {};
    device.usUsagePage = 0x01; // generic desktop
    device.usUsage = 0x06;     // keyboard
    device.dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    device.hwndTarget = this->window;
    if (!RegisterRawInputDevices(&device, 1, sizeof(device)))
    {
        error = "Raw Input: RegisterRawInputDevices failed";
        return false;
    }

    QCoreApplication::instance()->installNativeEventFilter(this);
    this->registered = true;
    return true;
}

bool RawInputHub::add(RawInputBackend *backend, QString &error)
{
    if (!ensureRegistered(error))
        return false;
    this->backends.append(backend);
    return true;
}

void RawInputHub::remove(RawInputBackend *backend)
{
    this->backends.removeAll(backend);
}

QString RawInputHub::deviceName(HANDLE device)
{
    if (!device)
        return QString();
    if (this->device_names.contains(device))
        return this->device_names.value(device);

    UINT chars = 0;
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, nullptr, &chars) != 0 || chars == 0)
        return QString();
    std::vector<wchar_t> buffer(chars + 1, 0);
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, buffer.data(), &chars) == static_cast<UINT>(-1))
        return QString();

    const QString name = QString::fromWCharArray(buffer.data());
    this->device_names.insert(device, name);
    return name;
}

bool RawInputHub::nativeEventFilter(const QByteArray &event_type, void *message, qintptr *result)
{
    Q_UNUSED(event_type);
    Q_UNUSED(result);

    MSG *msg = static_cast<MSG*>(message);
    if (!msg || msg->hwnd != this->window)
        return false;

    if (msg->message == WM_INPUT)
        handleInput(reinterpret_cast<HRAWINPUT>(msg->lParam));
    else if (msg->message == WM_INPUT_DEVICE_CHANGE)
        handleDeviceChange(msg->wParam, reinterpret_cast<HANDLE>(msg->lParam));

    // not consumed: DefWindowProc must clean up after WM_INPUT
    return false;
}

void RawInputHub::handleInput(HRAWINPUT input)
{
    UINT size = 0;
    if (GetRawInputData(input, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0)
        return;
    std::vector<BYTE> buffer(size);
    if (GetRawInputData(input, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size)
        return;

    const RAWINPUT *raw = reinterpret_cast<const RAWINPUT*>(buffer.data());
    if (raw->header.dwType != RIM_TYPEKEYBOARD)
        return;

    // no device handle: injected input (SendInput) or a virtual source
    const QString device = deviceName(raw->header.hDevice);
    if (device.isEmpty())
        return;

    const int key = linuxKeyCode(raw->data.keyboard);
    const bool pressed = !(raw->data.keyboard.Flags & RI_KEY_BREAK);

    const QList<RawInputBackend*> receivers = this->backends;
    for (RawInputBackend *backend : receivers)
    {
        if (backend->id == device)
            backend->owner->reportKey(key, pressed);
    }
}

void RawInputHub::handleDeviceChange(WPARAM change, HANDLE device)
{
    if (change != GIDC_REMOVAL)
        return;

    const QString name = this->device_names.take(device);
    if (name.isEmpty())
        return;

    const QList<RawInputBackend*> receivers = this->backends;
    for (RawInputBackend *backend : receivers)
    {
        if (backend->id == name)
        {
            // deferred: keyboardDisconnected() deletes the backend
            InputKeyboardRawController *controller = backend->owner;
            QMetaObject::invokeMethod(controller, [controller]() { controller->keyboardDisconnected(); }, Qt::QueuedConnection);
        }
    }
}

/*
 * Interception driver, loaded at runtime (the driver is optional).
 *
 * A filter on a keyboard routes all of its keys to us; we receive them on a
 * worker thread, independent of the GUI thread, and pass them on to Windows
 * unless the keyboard is locked. Keyboards without filter are not touched.
 * If the process ends, the driver drops our filters with the context handle.
 */
class InterceptionBackend;

class InterceptionHub
{
public:
    static constexpr int MAX_KEYBOARD = 10; // Interception devices 1..10 are keyboards
    
    // nullptr if interception.dll or the driver is missing
    static InterceptionHub *instance()
    {
        static InterceptionHub *hub = create();
        return hub;
    }
    
    // why instance() is nullptr
    static RawKeyboardProblem missing_problem;
    static QString missing_reason;
    
    QList<InputKeyboardRawMeta::Keyboard> keyboards();
    bool add(InterceptionBackend *backend, QString &error);
    void remove(InterceptionBackend *backend);
    
private:
    enum Mode { Off = 0, Listen = 1, Lock = 2 };
    
    using CreateContext = void* (*)();
    using Predicate = int (*)(int);
    using SetFilter = void (*)(void*, Predicate, unsigned short);
    using WaitWithTimeout = int (*)(void*, unsigned long);
    using Send = int (*)(void*, int, const void*, unsigned int);
    using Receive = int (*)(void*, int, void*, unsigned int);
    using GetHardwareId = unsigned int (*)(void*, int, void*, unsigned int);
    
    struct KeyStroke
    {
        unsigned short code;
        unsigned short state;
        unsigned int information;
    };
    // large enough for any InterceptionStroke (the mouse stroke is the largest)
    union Stroke
    {
        KeyStroke key;
        char raw[32];
    };
    
    static constexpr unsigned short KEY_UP = 0x01;
    static constexpr unsigned short KEY_E0 = 0x02;
    static constexpr unsigned short KEY_E1 = 0x04;
    static constexpr unsigned short FILTER_KEY_NONE = 0x0000;
    static constexpr unsigned short FILTER_KEY_ALL = 0xFFFF;
    
    HMODULE library = nullptr;
    void *context = nullptr;
    SetFilter set_filter = nullptr;
    WaitWithTimeout wait_with_timeout = nullptr;
    Send send = nullptr;
    Receive receive = nullptr;
    GetHardwareId get_hardware_id = nullptr;
    
    // index = Interception device; written on the GUI thread, read by the worker
    std::array<std::atomic<int>, MAX_KEYBOARD + 1> modes{};
    QList<InterceptionBackend*> backends;
    // lives in the GUI thread: target of the keys posted by the worker
    QObject receiver;
    bool worker_started = false;
    
    // interception_set_filter() takes a plain predicate: it selects this device
    static std::atomic<int> filter_device;
    static int isFilterDevice(int device)
    {
        return device == filter_device.load() ? 1 : 0;
    }
    
    static InterceptionHub *create();
    void updateMode(int device);
    void run();
    void deliver(int device, int key, bool pressed);
};

std::atomic<int> InterceptionHub::filter_device{0};
RawKeyboardProblem InterceptionHub::missing_problem = RawKeyboardProblem::LockNeedsInterception;
QString InterceptionHub::missing_reason;

class InterceptionBackend : public InputKeyboardRawBackend
{
public:
    InterceptionBackend(InputKeyboardRawController *owner, int device, bool lock)
        : owner(owner), device(device), lock(lock)
    {
    }
    
    ~InterceptionBackend() override
    {
        InterceptionHub::instance()->remove(this);
    }
    
    InputKeyboardRawController *owner;
    int device;
    bool lock;
};

InterceptionHub *InterceptionHub::create()
{
    HMODULE library = LoadLibraryW(L"interception.dll");
    if (!library)
    {
        missing_problem = RawKeyboardProblem::LockNeedsInterception;
        missing_reason = "interception.dll was not found next to vkeybd-qt.exe or on the PATH.";
        return nullptr;
    }
    
    CreateContext create_context = reinterpret_cast<CreateContext>(reinterpret_cast<void*>(GetProcAddress(library, "interception_create_context")));
    SetFilter set_filter = reinterpret_cast<SetFilter>(reinterpret_cast<void*>(GetProcAddress(library, "interception_set_filter")));
    WaitWithTimeout wait_with_timeout = reinterpret_cast<WaitWithTimeout>(reinterpret_cast<void*>(GetProcAddress(library, "interception_wait_with_timeout")));
    Send send = reinterpret_cast<Send>(reinterpret_cast<void*>(GetProcAddress(library, "interception_send")));
    Receive receive = reinterpret_cast<Receive>(reinterpret_cast<void*>(GetProcAddress(library, "interception_receive")));
    GetHardwareId get_hardware_id = reinterpret_cast<GetHardwareId>(reinterpret_cast<void*>(GetProcAddress(library, "interception_get_hardware_id")));
    
    if (!create_context || !set_filter || !wait_with_timeout || !send || !receive || !get_hardware_id)
    {
        qWarning() << "interception.dll found, but it is incomplete: keyboard lock not available";
        missing_problem = RawKeyboardProblem::LockNeedsInterception;
        missing_reason = "interception.dll was found, but it is not the Interception library (functions missing).";
        FreeLibrary(library);
        return nullptr;
    }
    
    // fails if the driver is not installed (or the reboot after installing is pending)
    void *context = create_context();
    if (!context)
    {
        qWarning() << "interception.dll found, but the Interception driver is not running: keyboard lock not available";
        missing_problem = RawKeyboardProblem::InterceptionNotRunning;
        missing_reason = "interception.dll was found, but the Interception driver is not running.";
        FreeLibrary(library);
        return nullptr;
    }
    
    InterceptionHub *hub = new InterceptionHub;
    hub->library = library;
    hub->context = context;
    hub->set_filter = set_filter;
    hub->wait_with_timeout = wait_with_timeout;
    hub->send = send;
    hub->receive = receive;
    hub->get_hardware_id = get_hardware_id;
    qInfo() << "Interception driver available: keyboards can be locked";
    return hub;
}

QList<InputKeyboardRawMeta::Keyboard> InterceptionHub::keyboards()
{
    // product names come from Raw Input, matched by vendor/product id
    QList<QString> raw_paths;
    UINT count = 0;
    if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) == 0 && count > 0)
    {
        std::vector<RAWINPUTDEVICELIST> devices(count);
        const UINT found = GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST));
        if (found != static_cast<UINT>(-1))
        {
            for (UINT i = 0; i < found; i++)
            {
                if (devices[i].dwType == RIM_TYPEKEYBOARD)
                    raw_paths.append(RawInputHub::instance().deviceName(devices[i].hDevice));
            }
        }
    }
    
    QList<InputKeyboardRawMeta::Keyboard> keyboards;
    for (int device = 1; device <= MAX_KEYBOARD; device++)
    {
        wchar_t hardware_id[512] = {};
        const unsigned int bytes = this->get_hardware_id(this->context, device, hardware_id, sizeof(hardware_id) - sizeof(wchar_t));
        if (bytes == 0)
            continue; // no keyboard behind this slot
        
        // e.g. "HID\VID_046D&PID_C31C&REV_6400&MI_00"
        const QString id = QString::fromWCharArray(hardware_id);
        QString vid_pid;
        const int vid = id.indexOf("VID_", 0, Qt::CaseInsensitive);
        if (vid >= 0)
            vid_pid = id.mid(vid, 17); // VID_xxxx&PID_xxxx
        
        QString product;
        if (!vid_pid.isEmpty())
        {
            for (const QString &path : raw_paths)
            {
                if (path.contains(vid_pid, Qt::CaseInsensitive))
                {
                    product = productName(path);
                    if (!product.isEmpty())
                        break;
                }
            }
        }
        if (product.isEmpty())
            product = "Keyboard";
        
        InputKeyboardRawMeta::Keyboard keyboard;
        // the slot tells identical keyboards apart
        keyboard.name = product + "\n@" + QString::number(device) + " " + (vid_pid.isEmpty() ? id : vid_pid);
        keyboard.id = "interception:" + QString::number(device);
        keyboards.append(keyboard);
    }
    return keyboards;
}

bool InterceptionHub::add(InterceptionBackend *backend, QString &error)
{
    if (backend->device < 1 || backend->device > MAX_KEYBOARD)
    {
        error = "Invalid Interception keyboard";
        return false;
    }
    
    if (!this->worker_started)
    {
        // never stopped: runs as long as the process
        std::thread worker([this]() { run(); });
        worker.detach();
        this->worker_started = true;
    }
    
    this->backends.append(backend);
    updateMode(backend->device);
    return true;
}

void InterceptionHub::remove(InterceptionBackend *backend)
{
    if (!this->backends.removeAll(backend))
        return;
    updateMode(backend->device);
}

void InterceptionHub::updateMode(int device)
{
    // locked if any user wants it locked (e.g. "Detect" listens at the same time)
    int mode = Off;
    for (const InterceptionBackend *backend : this->backends)
    {
        if (backend->device == device)
            mode = qMax(mode, backend->lock ? static_cast<int>(Lock) : static_cast<int>(Listen));
    }
    
    const int previous = this->modes[device].exchange(mode);
    if ((previous == Off) == (mode == Off))
        return;
    
    // GUI thread only, so filter_device is not shared with another call
    filter_device.store(device);
    this->set_filter(this->context, &InterceptionHub::isFilterDevice, (mode == Off) ? FILTER_KEY_NONE : FILTER_KEY_ALL);
    filter_device.store(0);
}

void InterceptionHub::run()
{
    // E1 1D 45 (Pause) arrives as two strokes
    std::array<bool, MAX_KEYBOARD + 1> pause_pending{};
    
    for (;;)
    {
        const int device = this->wait_with_timeout(this->context, 250);
        if (device < 1 || device > MAX_KEYBOARD)
            continue; // timeout, or not a keyboard
        
        Stroke stroke = {};
        if (this->receive(this->context, device, &stroke, 1) <= 0)
            continue;
        
        // the decision is made here, not on the GUI thread: a busy GUI must
        // neither block other keyboards nor let a locked keyboard through
        const int mode = this->modes[device].load();
        if (mode != Lock)
            this->send(this->context, device, &stroke, 1);
        if (mode == Off)
            continue;
        
        const unsigned short state = stroke.key.state;
        const bool pressed = (state & KEY_UP) == 0;
        int key = -1;
        if (state & KEY_E1)
        {
            pause_pending[device] = (stroke.key.code == 0x1D);
        }
        else if (pause_pending[device] && stroke.key.code == 0x45)
        {
            pause_pending[device] = false;
            key = 119; // KEY_PAUSE
        }
        else
        {
            key = scanCodeToLinux(stroke.key.code, (state & KEY_E0) != 0);
        }
        if (key < 0)
            continue;
        
        QMetaObject::invokeMethod(&this->receiver, [this, device, key, pressed]() { deliver(device, key, pressed); }, Qt::QueuedConnection);
    }
}

void InterceptionHub::deliver(int device, int key, bool pressed)
{
    const QList<InterceptionBackend*> receivers = this->backends;
    for (InterceptionBackend *backend : receivers)
    {
        if (backend->device == device)
            backend->owner->reportKey(key, pressed);
    }
}
}

bool InputKeyboardRawMeta::canLock()
{
    return InterceptionHub::instance() != nullptr;
}

QList<InputKeyboardRawMeta::Keyboard> InputKeyboardRawMeta::detectKeyboards()
{
    if (InterceptionHub *interception = InterceptionHub::instance())
        return interception->keyboards();
    
    QList<Keyboard> keyboards;
    
    UINT count = 0;
    if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) != 0 || count == 0)
        return keyboards;
    std::vector<RAWINPUTDEVICELIST> devices(count);
    const UINT found = GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST));
    if (found == static_cast<UINT>(-1))
        return keyboards;
    
    for (UINT i = 0; i < found; i++)
    {
        if (devices[i].dwType != RIM_TYPEKEYBOARD)
            continue;
        
        const QString path = RawInputHub::instance().deviceName(devices[i].hDevice);
        if (path.isEmpty())
            continue;
        
        QString product = productName(path);
        if (product.isEmpty())
            product = "Keyboard";
        
        Keyboard keyboard;
        keyboard.name = product + "\n@" + shortId(path);
        keyboard.id = path;
        keyboards.append(keyboard);
    }
    
    return keyboards;
}

std::unique_ptr<InputKeyboardRawBackend> createRawKeyboardBackend(InputKeyboardRawController *owner, const QString &id, KeyboardMode mode, QString &error, RawKeyboardProblem &problem)
{
    if (id.startsWith("interception:"))
    {
        InterceptionHub *interception = InterceptionHub::instance();
        if (!interception)
        {
            error = "The Interception driver is not available anymore.";
            problem = RawKeyboardProblem::InterceptionNotRunning;
            return nullptr;
        }
        std::unique_ptr<InterceptionBackend> backend = std::make_unique<InterceptionBackend>(owner, id.mid(13).toInt(), mode == KeyboardMode::lock);
        if (!interception->add(backend.get(), error))
            return nullptr;
        return backend;
    }
    
    // never pretend to lock
    if (mode == KeyboardMode::lock)
    {
        InterceptionHub::instance(); // fills in why it is missing
        error = InterceptionHub::missing_reason;
        problem = InterceptionHub::missing_problem;
        return nullptr;
    }
    
    std::unique_ptr<RawInputBackend> backend = std::make_unique<RawInputBackend>(owner, id);
    if (!RawInputHub::instance().add(backend.get(), error))
        return nullptr;
    return backend;
}
