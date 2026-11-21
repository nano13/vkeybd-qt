# Keyboard input and keyboard lock

Every keyboard section of KeyDeskMIDI has its own **input selector** (the combo
box next to the *Lock* button). It decides where the keys of that section
come from:

| Entry | Keys come from | Use it for |
|---|---|---|
| **[Disabled]** | nothing | sections that are only played by MIDI input |
| **Focused window** | keys typed while the KeyDeskMIDI window has the focus | a single keyboard, quick tests |
| **[Detect RAW event]** | press a key on any keyboard: that keyboard gets selected | finding the right device |
| *keyboard name* `@id` | exactly this physical keyboard, independent of the window focus | multi-keyboard setups, live performance |

All inputs report the *physical* key (Linux key codes), so the keyboard
layout (QWERTZ, AZERTY, ...) and Shift do not change which note a key plays.

## Focused window

The keys typed into the KeyDeskMIDI window. It cannot tell keyboards apart: all
keyboards of the computer play the same section. Every section that selects
*Focused window* gets these keys, so by default only the first section uses
it; the others start as *[Disabled]*.

*Lock* with *Focused window* grabs the keyboard for the KeyDeskMIDI window, so
the keys still arrive while another window of the desktop has the focus.

## Raw keyboards (one physical keyboard per section)

To play several sections with several keyboards, select a physical keyboard
in each section. Each keyboard then only plays its own section, whichever
window has the focus.

* **Listen** (not locked): KeyDeskMIDI reads the keys, and they also reach the
  rest of the system (the focused application, desktop shortcuts, ...).
* **Lock**: the keyboard belongs to KeyDeskMIDI alone. No other application and
  not the operating system see its keys — no shortcut, no Start menu, no
  accidental typing into some window during a performance.

A lock that cannot be delivered is never faked: KeyDeskMIDI shows a dialog
explaining why and what to do, switches the button back to *Lock* and uses
the keyboard without lock.

If a selected keyboard is unplugged, a dialog tells so. Plug it in again,
press the rescan button and select it again.

### Platform support

| Platform | Listen | Lock | How |
|---|---|---|---|
| Linux | yes | yes | evdev (`/dev/input/event*`), lock with `EVIOCGRAB` |
| Windows | yes | with the Interception driver | Interception driver if installed, otherwise Raw Input (listen only) |
| macOS | yes | no | IOKit HID Manager |

## Linux

### Permissions

Raw keyboards are read from `/dev/input/event*`. Normally only root and the
group `input` may read them:

```bash
sudo usermod -aG input $USER
# log out and in again (or reboot)
```

Be aware that every program of your user can then read all input devices.

### Lock

*Lock* takes the keyboard exclusively (`EVIOCGRAB`): X11, Wayland, the
console and all other programs stop seeing its keys until you unlock it or
quit KeyDeskMIDI. Other keyboards keep working normally.

Only one program can grab a keyboard. If the lock fails because the keyboard
is grabbed already, look for a second KeyDeskMIDI instance, `evtest --grab`, or
key remappers such as keyd, kmonad, kanata or input-remapper.

## Windows

* **Without the Interception driver:** KeyDeskMIDI reads the keyboards with
  the Raw Input API. Each keyboard plays its own section, also without the
  window focus, but the keys always reach Windows and the focused application
  too. Requesting *Lock* opens a dialog that explains the driver.
* **With the [Interception driver](interception.md)** (free, installed once):
  *Lock* keeps the keys of a keyboard away from Windows and all other
  applications. The keyboard list then shows the slot number of each
  keyboard, so identical keyboards can be told apart.

Installation, what to know before a performance, and troubleshooting:
[Keyboard lock on Windows: the Interception driver](interception.md) — also
in *Help > Windows Lock Driver (Interception)*.

## macOS

KeyDeskMIDI reads the keyboards with the IOKit HID Manager. Each keyboard plays
its own section, also without the window focus.

* **Input Monitoring:** macOS asks for this permission the first time a
  keyboard is opened. If it was denied, allow KeyDeskMIDI in *System Settings >
  Privacy & Security > Input Monitoring* and restart KeyDeskMIDI. The dialog
  shown by KeyDeskMIDI has a button that opens this page.
* **No lock:** macOS only lets programs running as root take exclusive control
  of a keyboard, so the keys always reach macOS and the focused application
  as well. For a locked live setup, use Linux, or Windows with Interception.
