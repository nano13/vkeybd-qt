#include "input_keyboard_xcb.h"

InputKeyboardXCB::InputKeyboardXCB(QObject *parent)
    : QObject{parent}
{
    Display * display;
    display = QX11Info::display();
    
    // https://stackoverflow.com/questions/38102221/qkeyevent-isautorepeat-not-working#38122074
    int supported;
    int result = XkbSetDetectableAutoRepeat(display, true, &supported);
    XFlush(display);
    if(!supported || !result) {
        qDebug() << "ERROR: Set Detectable Autorepeat FAILED";
    } else {
        qDebug() << "Setting Detectable Autorepeat SUCCESSFUL";
    }
}

bool InputKeyboardXCB::xcbEvent(const QByteArray &eventType, void *message, long *result)
{
    if (eventType == "xcb_generic_event_t")
    {
        xcb_generic_event_t* xev = static_cast<xcb_generic_event_t *>(message);
        
        if ((xev->response_type & ~0x80) == XCB_KEY_PRESS || (xev->response_type & ~0x80) == XCB_KEY_RELEASE)
        {
            
            bool is_autorepeat = isAutoRepeat(xev, message);
            if (!is_autorepeat)
            {
                if ((xev->response_type & ~0x80) == XCB_KEY_PRESS)
                {
                    xcb_key_press_event_t* kp = (xcb_key_press_event_t*)xev;
                    
                    const quint32 keycode = kp->detail;
                //      const quint32 keymod =
                //          static_cast<quint32>(kp->state & (ShiftMask | ControlMask |
                //                                            Mod1Mask | Mod4Mask));
                    qDebug() << "native press: " << keycode;
                    // "nativeEvent" gives the keycodes from the xserver;
                    // "rawKeyPressed" is written to take the keycodes directly from /dev/input/...
                    // the keycodes given by the xserver happen to be exactly off by 8,
                    // so we can simply compensate with -8
                    int keycode_raw = keycode - 8;
                    
                    //this->list_of_maintabs.first()->rawKeyPressed(keycode_raw);
                    emit rawKeyPressedSignal(keycode_raw);
                    
                    return true;
                }
                else if ((xev->response_type & ~0x80) == XCB_KEY_RELEASE)
                {
                    xcb_key_press_event_t* kp = (xcb_key_press_event_t*)xev;
                    const quint32 keycode = kp->detail;
                    
                    qDebug() << "native release: " << keycode;
                    int keycode_raw = keycode - 8;
                    
                    //this->list_of_maintabs.first()->rawKeyReleased(keycode_raw);
                    emit rawKeyReleasedSignal(keycode_raw);
                    
                    return true;
                }
            }
        }   
        
    }
    
    return false;
}

bool InputKeyboardXCB::isAutoRepeat(xcb_generic_event_t* xev, void *message)
{
    //auto kev = static_cast<xcb_key_press_event_t*>(message);
    //qDebug() << ", code: " << kev->detail << ", timestamp: " << kev->time << ", sequence:" << kev->sequence;
    
    bool autorepeat_detected = false;
    if (xev->response_type == XCB_KEY_PRESS)
    {
        auto kev = static_cast<xcb_key_press_event_t*>(message);
        //qDebug() << ", code: " << kev->detail << ", timestamp: " << kev->time << ", sequence:" << kev->sequence;
        
        if (this->list_of_keypresses.contains(kev->detail))
        {
            autorepeat_detected = true;
        }
        
        if (!this->list_of_keypresses.contains(kev->detail))
        {
            this->list_of_keypresses.append(kev->detail);
        }
    }
    else if (xev->response_type == XCB_KEY_RELEASE)
    {
        auto kev = static_cast<xcb_key_press_event_t*>(message);
        
        int index = this->list_of_keypresses.indexOf(kev->detail);
        this->list_of_keypresses.removeAt(index);
    }
    
    //qDebug() << "AUTOREPEAT: " << autorepeat_detected;
    
    return autorepeat_detected;
}
