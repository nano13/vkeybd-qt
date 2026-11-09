// Raw keyboard input on macOS: IOKit HID Manager
//
// One HID manager for all keyboards (usage page 0x01, usage 0x06),
// scheduled on the main run loop, so the callbacks run on the GUI thread.
// Every input value comes with its device. Reading keyboards needs the
// "Input Monitoring" permission (System Settings > Privacy & Security).
// Seizing a keyboard needs root, so there is no lock.

#include "input_keyboard_raw.h"

#include <QDebug>

#include <vector>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDUsageTables.h>

namespace
{
class HidBackend;

class HidHub
{
public:
    static HidHub &instance()
    {
        // never destroyed: lives as long as the process
        static HidHub *hub = new HidHub;
        return *hub;
    }

    bool add(HidBackend *backend, QString &error);
    void remove(HidBackend *backend);

    QList<InputKeyboardRawMeta::Keyboard> keyboards();

private:
    IOHIDManagerRef manager = nullptr;
    bool opened = false;
    QList<HidBackend*> backends;

    bool ensureCreated();
    bool ensureOpened(QString &error);

    static void inputValue(void *context, IOReturn result, void *sender, IOHIDValueRef value);
    static void deviceRemoved(void *context, IOReturn result, void *sender, IOHIDDeviceRef device);
};

class HidBackend : public InputKeyboardRawBackend
{
public:
    HidBackend(InputKeyboardRawController *owner, const QString &id)
        : owner(owner), id(id)
    {
    }

    ~HidBackend() override
    {
        HidHub::instance().remove(this);
    }

    InputKeyboardRawController *owner;
    QString id;
};

// HID usage (page 0x07) -> Linux key code; from hid_keyboard[] in Linux drivers/hid/hid-input.c
int linuxKeyCode(uint32_t usage)
{
    static const int table[0x68] = {
          -1,  -1,  -1,  -1,  30,  48,  46,  32,  18,  33,  34,  35,  23,  36,  37,  38,
          50,  49,  24,  25,  16,  19,  31,  20,  22,  47,  17,  45,  21,  44,   2,   3,
           4,   5,   6,   7,   8,   9,  10,  11,  28,   1,  14,  15,  57,  12,  13,  26,
          27,  43,  43,  39,  40,  41,  51,  52,  53,  58,  59,  60,  61,  62,  63,  64,
          65,  66,  67,  68,  87,  88,  99,  70, 119, 110, 102, 104, 111, 107, 109, 106,
         105, 108, 103,  69,  98,  55,  74,  78,  96,  79,  80,  81,  75,  76,  77,  71,
          72,  73,  82,  83,  86, 127, 116, 117,
    };

    if (usage < 0x68)
        return table[usage];

    // modifiers
    switch (usage)
    {
        case 0xE0: return 29;  // KEY_LEFTCTRL
        case 0xE1: return 42;  // KEY_LEFTSHIFT
        case 0xE2: return 56;  // KEY_LEFTALT
        case 0xE3: return 125; // KEY_LEFTMETA
        case 0xE4: return 97;  // KEY_RIGHTCTRL
        case 0xE5: return 54;  // KEY_RIGHTSHIFT
        case 0xE6: return 100; // KEY_RIGHTALT
        case 0xE7: return 126; // KEY_RIGHTMETA
        default:   return -1;
    }
}

QString stringProperty(IOHIDDeviceRef device, CFStringRef key)
{
    CFTypeRef value = IOHIDDeviceGetProperty(device, key);
    if (!value || CFGetTypeID(value) != CFStringGetTypeID())
        return QString();
    return QString::fromCFString(static_cast<CFStringRef>(value));
}

int intProperty(IOHIDDeviceRef device, CFStringRef key)
{
    CFTypeRef value = IOHIDDeviceGetProperty(device, key);
    int number = 0;
    if (value && CFGetTypeID(value) == CFNumberGetTypeID())
        CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberIntType, &number);
    return number;
}

// stable while the keyboard stays in the same port
QString deviceId(IOHIDDeviceRef device)
{
    return QString::number(static_cast<uint32_t>(intProperty(device, CFSTR(kIOHIDLocationIDKey))), 16).rightJustified(8, '0');
}

bool HidHub::ensureCreated()
{
    if (this->manager)
        return true;

    this->manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!this->manager)
        return false;

    CFMutableDictionaryRef matching = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    const int page = kHIDPage_GenericDesktop;
    const int usage = kHIDUsage_GD_Keyboard;
    CFNumberRef page_number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &page);
    CFNumberRef usage_number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
    CFDictionarySetValue(matching, CFSTR(kIOHIDDeviceUsagePageKey), page_number);
    CFDictionarySetValue(matching, CFSTR(kIOHIDDeviceUsageKey), usage_number);
    IOHIDManagerSetDeviceMatching(this->manager, matching);
    CFRelease(usage_number);
    CFRelease(page_number);
    CFRelease(matching);

    IOHIDManagerRegisterInputValueCallback(this->manager, &HidHub::inputValue, this);
    IOHIDManagerRegisterDeviceRemovalCallback(this->manager, &HidHub::deviceRemoved, this);
    IOHIDManagerScheduleWithRunLoop(this->manager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    return true;
}

bool HidHub::ensureOpened(QString &error)
{
    if (this->opened)
        return true;
    if (!ensureCreated())
    {
        error = "IOHIDManagerCreate failed";
        return false;
    }

    // shows the system prompt once; the user may have to restart the app afterwards
    if (__builtin_available(macOS 10.15, *))
        IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);

    const IOReturn result = IOHIDManagerOpen(this->manager, kIOHIDOptionsTypeNone);
    if (result != kIOReturnSuccess)
    {
        error = "cannot open the keyboards: allow vkeybd-qt in System Settings > Privacy & Security > Input Monitoring";
        return false;
    }
    this->opened = true;
    return true;
}

bool HidHub::add(HidBackend *backend, QString &error)
{
    if (!ensureOpened(error))
        return false;
    this->backends.append(backend);
    return true;
}

void HidHub::remove(HidBackend *backend)
{
    this->backends.removeAll(backend);
}

QList<InputKeyboardRawMeta::Keyboard> HidHub::keyboards()
{
    QList<InputKeyboardRawMeta::Keyboard> keyboards;
    if (!ensureCreated())
        return keyboards;

    CFSetRef devices = IOHIDManagerCopyDevices(this->manager);
    if (!devices)
        return keyboards;

    const CFIndex count = CFSetGetCount(devices);
    std::vector<const void*> values(static_cast<size_t>(count));
    CFSetGetValues(devices, values.data());

    for (const void *value : values)
    {
        IOHIDDeviceRef device = static_cast<IOHIDDeviceRef>(const_cast<void*>(value));

        QString product = stringProperty(device, CFSTR(kIOHIDProductKey));
        if (product.isEmpty())
            product = "Keyboard";

        InputKeyboardRawMeta::Keyboard keyboard;
        keyboard.id = deviceId(device);
        keyboard.name = product + "\n@" + keyboard.id;
        keyboards.append(keyboard);
    }

    CFRelease(devices);
    return keyboards;
}

void HidHub::inputValue(void *context, IOReturn result, void *sender, IOHIDValueRef value)
{
    Q_UNUSED(result);
    Q_UNUSED(sender);

    HidHub *hub = static_cast<HidHub*>(context);
    IOHIDElementRef element = IOHIDValueGetElement(value);
    if (IOHIDElementGetUsagePage(element) != kHIDPage_KeyboardOrKeypad)
        return;

    const int key = linuxKeyCode(IOHIDElementGetUsage(element));
    if (key < 0)
        return;
    const bool pressed = IOHIDValueGetIntegerValue(value) != 0;

    const QString id = deviceId(IOHIDElementGetDevice(element));
    const QList<HidBackend*> receivers = hub->backends;
    for (HidBackend *backend : receivers)
    {
        if (backend->id == id)
            backend->owner->reportKey(key, pressed);
    }
}

void HidHub::deviceRemoved(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
{
    Q_UNUSED(result);
    Q_UNUSED(sender);

    HidHub *hub = static_cast<HidHub*>(context);
    const QString id = deviceId(device);
    const QList<HidBackend*> receivers = hub->backends;
    for (HidBackend *backend : receivers)
    {
        if (backend->id == id)
        {
            // deferred: keyboardDisconnected() deletes the backend
            InputKeyboardRawController *controller = backend->owner;
            QMetaObject::invokeMethod(controller, [controller]() { controller->keyboardDisconnected(); }, Qt::QueuedConnection);
        }
    }
}
}

bool InputKeyboardRawMeta::canLock()
{
    return false;
}

QList<InputKeyboardRawMeta::Keyboard> InputKeyboardRawMeta::detectKeyboards()
{
    return HidHub::instance().keyboards();
}

std::unique_ptr<InputKeyboardRawBackend> createRawKeyboardBackend(InputKeyboardRawController *owner, const QString &id, KeyboardMode mode, QString &error)
{
    // seizing a keyboard needs a root helper; never pretend to lock
    if (mode == KeyboardMode::lock)
    {
        error = "Locking keyboards is not supported on macOS.";
        return nullptr;
    }

    std::unique_ptr<HidBackend> backend = std::make_unique<HidBackend>(owner, id);
    if (!HidHub::instance().add(backend.get(), error))
        return nullptr;
    return backend;
}
