# Keyboard lock on Windows: the Interception driver

On stage, a **locked** keyboard belongs to KeyDeskMIDI alone: no key reaches
Windows or another application — no Start menu, no shortcut, no accidental
typing into some window. On Windows, this needs the free **Interception**
driver.

## Why a driver?

Windows lets applications *listen* to each keyboard separately (Raw Input),
and that is what KeyDeskMIDI does without the driver: every keyboard plays its
own section, also without the window focus. But Windows has no way for an
application to *keep* the keys of one keyboard to itself. Every key always
reaches Windows and the focused application as well.

[Interception](https://github.com/oblitum/Interception) by Francisco Lopes is
a signed keyboard and mouse filter driver that closes this gap. With it, an
application receives the keys of each keyboard first and decides whether
Windows gets them:

* **Listen:** KeyDeskMIDI plays the keys and passes them on to Windows.
* **Lock:** KeyDeskMIDI plays the keys and keeps them away from Windows.

Keyboards that no section uses are not touched.

## Installation

1. Download **Interception.zip** from the
   [releases page](https://github.com/oblitum/Interception/releases).
2. Unpack it, open a command prompt **as administrator** in the folder
   `command line installer` and run `install-interception.exe /install`
3. **Reboot** Windows.
4. Copy **interception.dll** next to **keydesk-midi.exe**: from `library\x64`
   for a 64-bit KeyDeskMIDI, from `library\x86` for a 32-bit one.
5. Start KeyDeskMIDI and lock a keyboard.

KeyDeskMIDI loads `interception.dll` by itself when it needs it; nothing else
is needed, and without it, KeyDeskMIDI simply works without lock.

### Is it working?

* *Lock* works without a dialog, and the keys of the locked keyboard no
  longer type into other windows.
* With the driver, the keyboard list shows the slot number of each keyboard,
  e.g. `@3 VID_046D&PID_C31C`. This also tells identical keyboards apart.
* Started from a terminal, KeyDeskMIDI prints
  `Interception driver available: keyboards can be locked`.

### Uninstall

Run `install-interception.exe /uninstall` as administrator and reboot.

## Before a performance

* **Plug in all keyboards, then reboot.** The driver has 10 keyboard slots.
  Every unplug/replug — and every resume from hibernation — uses a new one.
  When they are used up, the keyboard stops working, also in Windows, until
  the next reboot. Do not hot-plug keyboards during the show.
* **KeyDeskMIDI keeps working when busy:** the decision to block or pass a key
  is made on a separate thread, so a busy window never lets a locked key
  through, and never holds up the other keyboards.
* **If KeyDeskMIDI quits or crashes**, the driver drops its filters and all
  keyboards work normally again.

## Good to know

* **Licence:** Interception is free for non-commercial use; commercial use
  needs a licence from its author. See its
  [licences](https://github.com/oblitum/Interception/tree/master/licenses).
* **Games:** the anti-cheat software of some games refuses to run while
  Interception is installed. On a dedicated music computer, this does not
  matter.
* **It is a kernel driver** from a third party. Install it from the official
  releases page only.

## Troubleshooting

| KeyDeskMIDI says | Cause | Fix |
|---|---|---|
| *needs the free Interception driver* | `interception.dll` not found | copy it next to `keydesk-midi.exe` |
| *the Interception driver is not running* | driver not installed, or the reboot after installing is still pending | install it, reboot |
| *interception.dll … is not the Interception library* | a wrong or damaged DLL | copy it again from Interception.zip |
| a keyboard stopped working completely | its 10 slots are used up (many replugs) | reboot |

A 64-bit KeyDeskMIDI cannot load the 32-bit `interception.dll` (and the other
way round): Windows then reports it as not found. Use the one from the
matching `library` folder.

See also: [Keyboards for live performance](keyboards.md).
