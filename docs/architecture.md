# Architecture (for developers)

```
input  ──►  MainTabs (one per keyboard section)  ──►  MidiEngine  ──►  InterfaceAudio  ──►  MIDI
            (current tab + checked tabs)              (presets,       (ALSA / JACK /
                                                       held notes)     libremidi / network)
GUI: MainWindow, Orgelwerk (one view per section, bound to the current tab), MIDIChannelSelector
```

## Core

* `preset_model.h` — `TabPreset`: everything a tab sounds like (16
  `ChannelSettings`, CC entries, master key shift and volume, ...). Plain data,
  no GUI.
* `preset_serializer.*` — `TabPreset` ⇄ INI files (layout
  `<keyboard>/<tab>/<channel>/key`, compatible with older saves). Duplicate CC
  entries of old files are dropped on load.
* `midi_engine.*` — `MidiEngine`, one per section: holds the presets of all
  tabs, compiles them into routes, and turns key presses into MIDI. Held notes
  are remembered per source key, so a release always stops exactly the notes
  its press started, even after tab or shift changes. MIDI input notes use a
  separate key range (`MIDI_INPUT_HELD_KEY_BASE`).
* `interface_audio.*` — the abstract MIDI backend: output ports per tab, one
  MIDI input per section (`dispatchMidiMessage()` turns incoming bytes into
  signals).
  * `interface_alsa.*`, `interface_jack.*` (lock-free ring buffers between the
    GUI and the JACK thread), `interface_libremidi.*`.

## Input

* `input_keyboard_window.*` — *Focused window*: Qt key events → Linux key
  codes via the native scan code (`linuxKeyCode()`; the place for the
  Windows/macOS translation once the GUI input is ported).
* `input_keyboard_raw.*` — raw keyboards, platform neutral:
  `InputKeyboardRawMeta` lists keyboards, `InputKeyboardRawController` opens
  one, removes auto-repeat, releases held keys when switching, and reports
  problems as `RawKeyboardProblem`.
  * `input_keyboard_raw_linux.cpp` — evdev, `EVIOCGRAB`.
  * `input_keyboard_raw_windows.cpp` — Interception (runtime-loaded
    `interception.dll`, worker thread decides pass/block) or Raw Input.
  * `input_keyboard_raw_macos.cpp` — IOKit HID Manager.
* `input_keyboard_select.*` — the selector, *Lock* button and detect mode of
  a section. `keyboard_problem_dialog.*` explains failed opens and locks.

A requested lock that cannot be delivered is an error, never a silent
fallback to listening.

## GUI

* `main_window.*` — creates the sections, routes *Focused window* keys to the
  sections that selected it, menu actions, save/load.
* `main_tabs.*` — one per section: tab handling, key bindings, network
  remote, creates the MIDI backend and the `MidiEngine`.
* `orgelwerk.*` — the view of the current tab (one per section, rebound on
  tab switch; nothing is built per tab).
* `midi_channel_selector.*` — the channel editor, created on first use and
  shared by all tabs of a section.

## Tests

`tests/tst_core.cpp` (engine, serializer, MIDI input),
`tests/tst_keyboard_window.cpp` (focused-window key tracking),
`tests/tst_keyboard_layout.cpp` (PC keyboard mapping and piano widget).
Run them with `./compile_linux_tests.sh`.
