# vkeybd-qt
MIDI software controller for the PC keyboard, with a lot of functions —
made for playing live.

![EditorView](screenshots/main_window.png)

![EditorView](screenshots/midi_channels.png)

## What it does

* **Several keyboard sections** side by side (`-n`), each with its own input,
  its own MIDI ports and its own instrument tabs.
* **Tabs** (`F01-1`, `F02-1`, ...): complete presets of 16 MIDI channels
  each. Switch with the function keys; check several tabs to play them
  together with one key.
* **One physical keyboard per section**, and **keyboard lock**: a locked
  keyboard belongs to vkeybd-qt alone, so no key reaches the operating
  system or another application during a performance.
* **MIDI input per section**: play the tabs of a section from a MIDI keyboard
  or a sequencer, with all their instruments at once.
* **Outputs:** ALSA, JACK, libremidi (cross-platform), or network (remote
  control another vkeybd-qt instance).
* Pedals, pitch bend, volume, octave and key shift on the PC keyboard.

## Documentation

* [Playing](docs/playing.md) — key bindings.
* [Keyboard input and keyboard lock](docs/keyboard-input.md) — focused
  window vs. raw keyboards, locking on Linux, Windows (Interception driver)
  and macOS, permissions.
* [MIDI output, MIDI input and tabs](docs/midi.md) — ALSA, JACK, libremidi,
  network, ports, checked tabs, MIDI input.
* [Architecture](docs/architecture.md) — for developers.

## Platforms

Linux is the main platform. The keyboard input and the libremidi output are
prepared for Windows and macOS; the rest of the port is in progress.

| | Linux | Windows | macOS |
|---|---|---|---|
| raw keyboards (one per section) | yes | yes | yes |
| keyboard lock | yes | with the [Interception driver](docs/keyboard-input.md#interception-driver-needed-for-lock) | no |
| MIDI output | ALSA, JACK, libremidi | — | libremidi (CoreMIDI) |

## Usage

```bash
vkeybd-qt [-n <keyboards>] [-l <layers>] [-o alsa|jack|libremidi|network]
```

| Option | Meaning |
|---|---|
| `-n`, `--number-of-keyboards` | number of keyboard sections (1–10) |
| `-l`, `--number-of-layers` | tab layers: each layer has 12 tabs (`F01`…`F12`) |
| `-o`, `--output` | MIDI system; default: `default/output` in `config.ini` |
| `-q`, `--quicksave-path` | file for QuickSave/QuickLoad |

### Configuration

`config.ini` and the quicksave files live in the application data directory,
on Linux `~/.local/share/vkeybd-qt/`. Defaults are written on the first
start (`number-of-keyboards`, `number-of-layers`, `output`, `notify`, ...).

## Build (Linux, CMake + Ninja)

Install CMake (3.21+), Ninja, a C++20 compiler, pkg-config, Qt 6 (Core,
Gui, Widgets, Svg, SvgWidgets, Network, DBus, Test), ALSA development files,
and JACK development files. The libremidi backend is built by default; CMake
uses an installed libremidi or downloads it (needs Git and network access the
first time). Disable it with `-DVKEYBD_WITH_LIBREMIDI=OFF`. For Debian/Ubuntu, an example is:

```bash
sudo apt install cmake ninja-build g++ pkg-config qt6-base-dev qt6-svg-dev \
    libasound2-dev libjack-jackd2-dev
```

Build the application without launching it:

```bash
./compile_linux.sh debug       # build-debug/vkeybd-qt (default)
./compile_linux.sh release     # build-release/vkeybd-qt
```

Build **and run** the application (legacy entry point):

```bash
./compile.sh debug            # or: ./compile.sh release
```

Build and run the GUI-independent QtTest suite:

```bash
./compile_linux_tests.sh debug
./compile_linux_tests.sh release
```

Tests are registered with CTest, so you may also run
`ctest --test-dir build-debug --output-on-failure`.
If you want to test an existing INI file, use
`VKEYBD_INI=/path/to/save.ini ./compile_linux_tests.sh debug`.

The new source layout is `src/` (application C++ code, headers and Qt
resources), `tests/` (unit tests), and `assets/` (runtime resources).
CMake files are `CMakeLists.txt` at the project root, in `src/` and in
`tests/`. The obsolete `.pro` projects are no longer needed. All generated
files, including Qt moc/rcc outputs, test executables and Make/Ninja metadata,
stay under `build-debug/` or `build-release/`, which are ignored by Git and
excluded from source snapshot ZIPs.

## Patch ZIPs and source snapshots

From any working directory, run:

```bash
./tools/zip_vkeybd_project.sh [output.zip]
./tools/apply_patch.sh /path/to/vkeybd-<description>.zip
```

The ZIP snapshot defaults to `../vkeybd.zip` (beside the project checkout).
It contains Git-tracked and non-ignored untracked source files, excluding
generated builds, known secrets, and compiled helper executables. Its paths
always begin with `vkeybd-qt/`, regardless of the checkout folder name.

Patch ZIPs must be named `vkeybd-<description>.zip` and contain only
changed or added files below `vkeybd-qt/`. To delete files, optionally include
a UTF-8 `vkeybd-qt/.vkeybd-patch-delete.txt` listing relative file paths, one
per line. The patch applicator validates paths, refuses symlinks and unsafe
entries, writes changed files using atomic replacements, refreshes their
modification times, and regenerates `../vkeybd.zip` on success. Requires
Python 3 and Git.

Run `./tests/test_patch_tooling.sh` to test this machinery in temporary Git
checkouts.

