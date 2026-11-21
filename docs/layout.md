# The keyboard layout: chromatic button accordion

By default, vkeybd-qt lays out the notes on the computer keyboard like the
buttons of a **chromatic button accordion**. If you have never played one:
you do not need to. The system is quick to learn, because it is completely
regular — it is **isomorphic**: every chord and every scale has one fixed
pattern of keys, the same in all twelve keys.

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
  same note under two fingers gives you alternative fingerings, and patterns
  that start in row 3 continue there (see below).

The PC keyboard display of vkeybd-qt shows the note on every key.

## Isomorphic: one pattern for every key

The accordion layout is **isomorphic**: the same musical interval is always
the same step on the keyboard, wherever you are. On the computer keyboard:

| Step on the keyboard | Interval |
|---|---|
| one key to the **right** | minor third (+3 semitones) |
| the key **down-right** (next row) | semitone (+1) |
| the key **down-left** (next row) | whole tone down (−2) |
| four keys to the right | octave (+12) |

This holds across all rows — from row 3, *down-right* continues into row 4.

So a chord or a scale is not a set of notes you have to find again in every
key: it is **one pattern of keys**, a shape for the hand. **Where you put the
hand decides the key**: the key under the root finger is the root of the
chord. Put the major pattern on `1`, and you play B major; on `2`, D major;
on `Q`, C major — the same fingers, the same movement.

### Chord patterns

`R` is the root — the key you put the pattern on. The rows are drawn
staggered like on the keyboard; each `·` is a key. The examples start on `1`
(B), German layout.

**Major** — on `1`: 1, W, E

```
row 1      R   ·   ·   ·
row 2        ·   3   5   ·
```

**Minor** — on `1`: 1, 2, E

```
row 1      R   ♭3  ·   ·
row 2        ·   ·   5   ·
```

**Diminished** — on `1`: 1, 2, 3

```
row 1      R   ♭3  ♭5  ·
```

**Augmented** — on `1`: 1, W, D

```
row 1      R   ·   ·   ·
row 2        ·   3   ·   ·
row 3         ·   ·   ♯5  ·
```

**Dominant 7th** — on `1`: 1, W, E, R

```
row 1      R   ·   ·   ·   ·
row 2        ·   3   5   ♭7  ·
```

**Major 7th** — on `1`: 1, W, E, F

```
row 1      R   ·   ·   ·   ·
row 2        ·   3   5   ·   ·
row 3         ·   ·   ·   7   ·
```

**Minor 7th** — on `1`: 1, 2, E, R

```
row 1      R   ♭3  ·   ·   ·
row 2        ·   ·   5   ♭7  ·
```

### The same pattern in every key

| Chord | Keys | |
|---|---|---|
| B major | 1, W, E | root in row 1 |
| C major | Q, S, D | the same pattern, one row lower (+1 semitone) |
| C♯ major | A, X, C | one row lower again: row 4 continues below row 3 |
| D major | 2, E, R | the B major pattern, one key to the right (+3) |

Row 4 has the same notes as row 1: for a root there, use the key in row 1.

Learn a piece in one key, and you can play it in all twelve: move the hand,
keep the pattern. Transposing on the fly is just as easy — or use the key
shift, and keep the hand where it is.

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
