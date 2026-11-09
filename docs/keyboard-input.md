# Keyboard input and keyboard lock

Every keyboard section of vkeybd-qt has its own **input selector** (the combo
box next to the *Lock* button). It decides where the keys of that section
come from:

| Entry | Keys come from | Use it for |
|---|---|---|
| **[Disabled]** | nothing | sections that are only played by MIDI input |
| **Focused window** | keys typed while the vkeybd-qt window has the focus | a single keyboard, quick tests |
| **[Detect RAW event]** | press a key on any keyboard: that keyboard gets selected | finding the right device |
| *keyboard name* `@id` | exactly this physical keyboard, independent of the window focus | multi-keyboard setups, live performance |

All inputs report the *physical* key (Linux key codes), so the keyboard
layout (QWERTZ, AZERTY, ...) and Shift do not change which note a key plays.

## Focused window

The keys typed into the vkeybd-qt window. It cannot tell keyboards apart: all
keyboards of the computer play the same section. Every section that selects
*Focused window* gets these keys, so by default only the first section uses
it; the others start as *[Disabled]*.

*Lock* with *Focused window* grabs the keyboard for the vkeybd-qt window, so
the keys still arrive while another window of the desktop has the focus.

## Raw keyboards (one physical keyboard per section)

To play several sections with several keyboards, select a physical keyboard
in each section. Each keyboard then only plays its own section, whichever
window has the focus.

* **Listen** (not locked): vkeybd-qt reads the keys, and they also reach the
  rest of the system (the focused application, desktop shortcuts, ...).
* **Lock**: the keyboard belongs to vkeybd-qt alone. No other application and
  not the operating system see its keys — no shortcut, no Start menu, no
  accidental typing into some window during a performance.

A lock that cannot be delivered is never faked: vkeybd-qt shows a dialog
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
quit vkeybd-qt. Other keyboards keep working normally.

Only one program can grab a keyboard. If the lock fails because the keyboard
is grabbed already, look for a second vkeybd-qt instance, `evtest --grab`, or
key remappers such as keyd, kmonad, kanata or input-remapper.

## Windows

### Without the Interception driver

vkeybd-qt reads the keyboards with the Raw Input API. Each keyboard plays its
own section, also without the window focus, but the keys always reach
Windows and the focused application too. *Lock* is not possible: requesting
it opens a dialog with the installation steps below.

### Interception driver (needed for Lock)

[Interception](https://github.com/oblitum/Interception) is a signed keyboard
and mouse filter driver by Francisco Lopes. With it, a normal application can
receive the keys of each keyboard and decide whether Windows gets them.
vkeybd-qt uses it for *Listen* (keys are passed on to Windows) and *Lock*
(keys are kept away from Windows).

Installation:

1. Download **Interception.zip** from the
   [releases page](https://github.com/oblitum/Interception/releases).
2. Open a command prompt **as administrator** in the folder
   `command line installer` and run:
   ```bat
   install-interception.exe /install
   ```
3. **Reboot** Windows.
4. Copy **interception.dll** next to **vkeybd-qt.exe**: from `library\x64`
   for a 64-bit vkeybd-qt, from `library\x86` for a 32-bit one.
5. Start vkeybd-qt. The log says
   `Interception driver available: keyboards can be locked`.

vkeybd-qt loads `interception.dll` at runtime; it is not needed for building
and nothing breaks without it. To uninstall the driver, run
`install-interception.exe /uninstall` as administrator and reboot.

With the driver, the keyboard list shows the Interception slot number of each
keyboard (`@3 VID_046D&PID_C31C`), so identical keyboards can be told apart.

Things to know before a performance:

* **Device limit:** the driver has 10 keyboard slots. Every unplug/replug (and
  resume from hibernation) uses a new one. When they are used up, the
  keyboard stops working — also in Windows — until the next reboot. Plug in
  all keyboards, reboot, and do not hot-plug during the show.
* **Licence:** free for non-commercial use (LGPL library, redistributable
  driver binaries); commercial use needs a licence from the author. See the
  [licences](https://github.com/oblitum/Interception/tree/master/licenses).
* Locked keys are kept away by a separate thread of vkeybd-qt, so a busy
  window never lets them through. If vkeybd-qt quits or crashes, the driver
  drops its filters and the keyboards work normally again.
* Anti-cheat software of some games refuses to run while Interception is
  installed.

## macOS

vkeybd-qt reads the keyboards with the IOKit HID Manager. Each keyboard plays
its own section, also without the window focus.

* **Input Monitoring:** macOS asks for this permission the first time a
  keyboard is opened. If it was denied, allow vkeybd-qt in *System Settings >
  Privacy & Security > Input Monitoring* and restart vkeybd-qt. The dialog
  shown by vkeybd-qt has a button that opens this page.
* **No lock:** macOS only lets programs running as root take exclusive control
  of a keyboard, so the keys always reach macOS and the focused application
  as well. For a locked live setup, use Linux, or Windows with Interception.
