# MIDI output, MIDI input and tabs

## Keyboard sections, tabs and layers

vkeybd-qt shows one or more **keyboard sections** (`-n`, up to 10). Each
section has its own input (see [keyboard input](keyboard-input.md)), its own
MIDI ports and its own set of **tabs**.

A tab is a complete instrument preset: 16 MIDI channels, each with its own
instrument (bank/program), volume, pan, key shift, velocity, CC values and so
on. Tabs are named after the function keys and the layer: `F01-1` … `F12-1`
is layer 1, `F01-2` … `F12-2` layer 2, and so on (`-l` sets the number of
layers).

* **Left click** on a tab (or its function key) makes it the current tab.
* **Right click** checks or unchecks a tab. All checked tabs sound
  together with the current one, so one key can play several presets at
  once. The last checked tab cannot be unchecked.

Tabs and checked tabs belong to their keyboard section: playing one section
never sounds the tabs of another section.

## Outputs

The output system is chosen with `-o` / `--output`, or by `default/output` in
`config.ini` (initially `jack`).

| Output | Clients and ports |
|---|---|
| `alsa` | one ALSA sequencer client `vkeybd-qt` per section, one output port per tab (`1-F01-1`, ...); the MIDI input is a client `vkeybd-qt-in` |
| `jack` | one JACK client per section (`vkeybd-qt-jack-0`, `-1`, ...), one MIDI output port per tab |
| `libremidi` | one client per section (`vkeybd-qt-1`, ...), one virtual output port per tab; see below |
| `network` | no MIDI: the keys are sent via UDP to another vkeybd-qt instance |

Connect the tab ports to your synthesizers with your patchbay (qjackctl,
Carla, `aconnect`, ...). Each tab port uses the channels configured in the
tab.

### libremidi

[libremidi](https://github.com/celtera/libremidi) is a cross-platform MIDI
library, meant as the backend for Windows and macOS. It is built by default
(`-DVKEYBD_WITH_LIBREMIDI=OFF` disables it); CMake uses an installed
libremidi or downloads v5.3.1.

* **Linux:** JACK is used if a JACK server (or PipeWire's JACK support) is
  running, otherwise the ALSA sequencer. One client per section holds all its
  ports. The API is chosen once for all sections.
* **macOS:** CoreMIDI virtual ports.
* **Windows:** not done yet — the classic Windows MIDI API (WinMM) has no
  virtual ports, so another solution is needed there.

The downloaded libremidi gets a one-line fix applied at configure time (an
inverted semaphore condition in its JACK backend, which made debug builds
abort). An installed libremidi does not get this fix.

### Network

With `-o network`, a vkeybd-qt instance does not produce MIDI itself: every
key press and release is sent as UDP datagram (`keypress/<code>`,
`keyrelease/<code>`) to the IP address and port of the network section. The
receiving instance (with a real MIDI output) listens on its own network
section address (default `127.0.0.1`, port `20020`) and plays the keys as if
they were typed there. Show the network section with *View > Show Network
Section*.

## MIDI input per section

Every section also has a **MIDI input port** (`1-MIDI-in`, `2-MIDI-in`, ...)
with ALSA, JACK and libremidi. Connect a MIDI keyboard or a sequencer to it,
and the section plays the incoming notes through all its sounding tabs —
with their instruments, key shifts, channels and velocities. One MIDI
keyboard can so play several presets at once, just like the PC keyboard.

* Notes of all 16 incoming channels are accepted.
* The incoming velocity is scaled by the velocity of each tab channel.
* CC 64 (sustain), CC 66 (sostenuto) and CC 67 (soft) act like the pedal keys.
* Other controllers and pitch bend are forwarded to the sounding tabs.

## Saving

*File > Save/Load Instruments* writes all tabs of all sections to an INI file;
*QuickSave/QuickLoad* uses a fixed file per number of sections (in the
configuration directory, see the [README](../README.md#configuration)).
