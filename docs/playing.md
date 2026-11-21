# Playing

The keys below work with every keyboard input (focused window or raw
keyboard) and act on the section of that keyboard. They are physical keys:
the keyboard layout does not matter.

| Key | Action |
|---|---|
| letter, digit and punctuation keys | notes (see the PC keyboard section of the window) |
| F1 … F12 | switch to tab `Fxx` of the active row; held notes keep sounding until released, the next presses play the new tab |
| Esc + F1 … F12 | switch the active row (layer); or click the row button `1`, `2`, … in front of the row |
| Menu + F1 … F12 | add the tab to the sounding tabs, or remove it (like a right click on the tab) |
| Menu + Esc (tap Esc) | back to the current tab alone (no panic: it keeps sounding) |
| Menu held: Esc + F1 … F12, then F-keys | collect tabs across rows: switch the row and add/remove its tabs without letting go of Menu |
| Esc | panic: all notes off and all sound off, on all tabs of the section |
| Delete | all sound off on all tabs of the section |
| Insert | resend all MIDI settings of the tab |
| Space | sustain pedal |
| Alt (left or right) | sostenuto pedal |
| Super (Windows key) | soft pedal |
| left / right Ctrl | octave down / up |
| Left / Right | pitch bend down / up while held |
| End / Home | pitch bend down / up, stays |
| Down / Up | volume down / up while held |
| Page Down / Page Up | volume down / up, stays |
| Print / Pause | global key shift −1 / +1 (all sections) |
| Scroll Lock | global key shift back to the start value |

The hotkeys for octave shift, pitch bend, volume and the pedals act on the
current tab **and all checked tabs** (right click on a tab) — everything that
sounds:

* **Volume — group DCA:** while more than one tab sounds, a *Group DCA* fader
  appears below the tab rows, and the volume hotkeys drive it. It scales all
  sounding tabs by the same factor, so their volume ratios (their balance)
  stay, like a DCA on a mixing desk. 100 % = the tabs' own volumes. A tab that
  hits 0 or 120 comes back to its exact share when the DCA goes back.
* **Octave shift:** the Ctrl keys move all sounding tabs by the same number
  of semitones.

The controls of a tab itself — its volume slider, its key shift buttons —
change only that tab. A tab changed that way keeps its new level in the group.

For a live performance, give each section its own physical keyboard and
**lock** it, so none of these keys (or any other) reaches the operating
system: see [keyboard input](keyboard-input.md).
