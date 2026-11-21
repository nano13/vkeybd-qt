# Keymaps and keyboard forms

What each key of a keyboard section plays is its **keymap**. The default is the
chromatic button accordion layout (see [Keyboard Layout](layout.md)), but any
other arrangement is possible: another pattern, or every key set by hand —
isomorphic or not.

Open the editor with *Tools > Keymap Editor...* (with several keyboard
sections, choose the one to edit).

## Keyboard forms: ISO, ANSI, JIS

KeyDeskMIDI reads the **position** of a key, not the character printed on it.
QWERTY, QWERTZ and AZERTY keyboards are therefore all the same here. What does
matter is the physical form, because some keys only exist on one of them:

| Form | Typical for | Difference |
|---|---|---|
| **ISO** | most of Europe | a `<` key next to the left Shift, a tall Enter with a key left of it |
| **ANSI** | US, much of Asia | no key next to the wide left Shift, backslash above Enter |
| **JIS** | Japan | ¥ in the number row, ろ next to the right Shift, three keys around the space bar |

The form is set per keyboard section, in the editor. **Automatic** (the
default) starts with a guess from the system (keyboard type on macOS and
Windows, otherwise the language) and corrects itself as soon as a key is
pressed that only one form has — the `<` key proves ISO, the Japanese keys
prove JIS. An ANSI keyboard has no such key: if the guess says ISO for an ANSI
keyboard, choose ANSI in the editor.

The PC keyboard display shows the keys of the form, where they really are.

## Patterns

A pattern fills all keys at once. *First key* is the note of the key left of
`1` (for the piano pattern: the note of `Z`); C4 is middle C.

| Pattern | Arrangement |
|---|---|
| **Accordion C-system** | rows of minor thirds, the rows a semitone apart — the default |
| **Accordion B-system** | the same, the rows a semitone apart the other way (bayan) |
| **Jankó** | whole tones in each row, the rows a semitone apart |
| **Wicki-Hayden** | whole tones in each row, a fourth between neighbouring rows |
| **Piano (tracker style)** | `Z` row white keys and `A` row black keys for one octave, `Q` row and number row for the next |
| **None** | only the keys set by hand sound |

The grid patterns follow the form: on an ANSI keyboard every key gets the
note of its real position.

## Setting single keys

Select a key — press it on the keyboard while the editor window is active
(raw keyboards), or click it — then choose its note, or *silent*. *Learn from
MIDI* takes the note from a MIDI keyboard on the section's MIDI input. Keys set
by hand have an orange corner; *Back to Pattern* undoes one, *Clear Keys Set by
Hand* all of them.

Keys set by hand belong to the key, not to the position: they stay the same
on every form. A key the keyboard does not have keeps its note in the keymap
and plays again on a keyboard that has it.

The function keys (Ctrl for the octaves, the pedals on Alt, Windows key and
space bar, Menu for the tabs) keep their functions and get no notes.

## Sounding octaves on the PC keyboards

*Show the sounding octaves on the PC keyboards* switches the small line on
the keys of the main window (what each key sounds as right now; see
[Keyboard Layout](layout.md)). It is one setting for all keyboards, takes
effect at once (not with *Apply*), and is the same as *View > Show Sounding
Notes on the PC Keyboards* and `pc-keyboard-sounding-notes` in `config.ini`.

## Apply, files

The editor works on a copy: *Apply* (or *OK*) hands it to the keyboard
section. While the editor is open, its keyboard keeps working normally as soon
as another window is active — so *Apply*, click the main window and try.

Each section remembers its keymap and form in the application data folder
(`keyboard-1.json`, `keyboard-2.json`, …, next to `config.ini`). *Save As...*
and *Load...* write and read keymap files to share them; they are in the
`keymaps` folder there by default. A keymap file is plain JSON:

```json
{
    "keydesk-midi-keymap": 1,
    "name": "my layout",
    "pattern": "accordion-c",
    "start": "G#2",
    "keys": { "86": "C3", "30": null }
}
```

`keys` holds the keys set by hand by their key codes (Linux input codes, e.g.
30 = `A`); `null` is silent.
