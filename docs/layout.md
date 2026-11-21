# The keyboard layout: chromatic button accordion

By default, vkeybd-qt lays out the notes on the computer keyboard like the
buttons of a **chromatic button accordion**. If you have never played one:
you do not need to. The system is quick to learn, because it is completely
regular.

## How the notes are arranged

The four letter and number rows of the computer keyboard are the button rows
of the accordion:

| Row | Keys | Notes (default) |
|---|---|---|
| 1 | `^`/`` ` ``, 1, 2, 3, … Backspace | G♯, B, D, F, G♯, B, … |
| 2 | Tab, Q, W, E, … Enter | A, C, E♭, F♯, A, C, … |
| 3 | Caps Lock, A, S, D, … `#` | B♭, C♯, E, G, B♭, C♯, … |
| 4 | Shift, `<`, Y/Z, X, … Shift | G♯, B, D, F, … — the same notes as row 1 |

* **Along a row**, every key is a **minor third** (3 semitones) higher than
  the one before. Four keys to the right is **one octave**.
* **From row to row**, the notes go up by a **semitone**: row 2 is one
  semitone above row 1, row 3 one above row 2. Together, the three rows
  contain every note of the chromatic scale.
* **Row 4 repeats row 1**, like the extra rows of a five-row accordion: the
  same note under two fingers gives you alternative fingerings, and shapes
  that start in row 3 continue there (see below).

The PC keyboard display of vkeybd-qt shows the note on every key.

## One shape for every key

Because the system is so regular, a chord or a scale has **one shape**, and
that shape is the same in every key. Move it one key to the right, and you
play it a minor third higher; move it one row down, and you play it a
semitone higher. Learn a piece in one key, and you can play it in all twelve.

A major chord, for example (German layout):

| Chord | Keys | Rows |
|---|---|---|
| B major | 1, W, E | root in row 1, third and fifth in row 2 |
| C major | Q, S, D | the same shape, one row lower |
| C♯ major | A, X, C | the same shape again: row 4 continues below row 3 |
| D major | 2, E, R | the B major shape, one key to the right |

## Range: more than three octaves per keyboard

The rows reach from G♯ up to C — **three octaves and a major third** on one
computer keyboard, more than most small controller keyboards offer.

And the range moves with you:

* **Left Ctrl:** one octave down. **Right Ctrl:** one octave up.
* Notes you hold keep sounding while you shift; the next notes you play are
  in the new octave. With a bit of practice, you shift octaves in the middle
  of a song, between two phrases, without a gap.
* While several tabs sound together, the octave keys move all of them.

The key shift buttons of a tab (and Print / Pause / Scroll Lock for all
sections) transpose in semitones.

## Two hands, two keyboards

vkeybd-qt can show several keyboard sections side by side (`-n 2`, `-n 3`,
…), each with its own physical keyboard, its own tabs and its own sounds —
like the **manuals of an organ**. Play one keyboard with the left hand and
another with the right: bass or accompaniment on one, melody or solo
registration on the other, each with the full range and its own octave keys.

How to connect and lock several keyboards: *Keyboard input and keyboard
lock* (`docs/keyboard-input.md`). Which keyboards work well:
[Keyboards for live performance](keyboards.md).
