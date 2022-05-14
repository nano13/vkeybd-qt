#ifndef ENUMS_H
#define ENUMS_H

#include <QObject>

enum class OutputSystem
{
    Alsa,
    Jack,
    Network,
};

enum class MIDISignalTypes
{
    KeyPress,
    KeyRelease,
    ToggleKeyboardLock ,
    Panick,
    StopAll,
    ShowMenu,
    ResendMIDISettings,
    
    SustainPressed,
    SustainReleased,
    
    SostenutoPressed,
    SostenutoReleased,
    
    SoftPressed,
    SoftReleased,
    
    VolumeLowerPressed,
    VolumeHigherPressed,
    VolumeReleased,
    
    PitchLowerPressed,
    PitchHigherPressed,
    PitchReleased,
};

enum class TabNames
{
    F1,
    F2,
    F3,
    F4,
    
    F5,
    F6,
    F7,
    F8,
    
    F9,
    F10,
    F11,
    F12
};

enum KeysRaw : uint8_t
{
    Escape = 1,
    Insert = 110,
    Delete = 111,
    Menu = 127,
    
    Alt = 56,
    Space = 57,
    Up = 103,
    Down = 108,
    Left = 105,
    Right = 106,
    PageUp = 104,
    PageDown = 109,
    Control = 29,
    Super_L = 125,
    Super_R = 125,
    
    F1 = 59,
    F2 = 60,
    F3 = 61,
    F4 = 62,
    
    F5 = 63,
    F6 = 64,
    F7 = 65,
    F8 = 66,
    
    F9 = 67,
    F10 = 68,
    F11 = 87,
    F12 = 88,
};

#endif // ENUMS_H
