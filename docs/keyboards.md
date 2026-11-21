# Keyboards for live performance

vkeybd-qt makes the computer keyboard a playable instrument. On stage, an
instrument has to be reliable — and whether every note arrives depends on the
keyboard you play. Laptop and office keyboards are fine for getting started;
for performing, choose a keyboard as described below.

## An organ-like instrument

Computer keys are switches: a key is pressed or released, nothing in between.
Every note sounds at the velocity set for its channel — the same, however the
key is struck. This gives vkeybd-qt the character of an organ, and not so
much that of a piano:

* **Registration:** the tabs are your stops. Switch them with the F-keys,
  combine several with Menu + F-key, change them mid-phrase without
  interrupting the notes you hold.
* **Phrasing** comes from articulation — legato, staccato, note lengths,
  the pedals.
* **Dynamics** are played with the volume keys, like with an organ's swell
  pedal — and they are very much part of playing: hold **Up/Down** for a
  swell or a fade that springs back by itself when you let go (how fast is
  set with the tether slider), use **Page Up/Page Down** for a change that
  stays. While several tabs sound, the keys drive the group DCA, so the
  whole registration gets louder or softer together and keeps its balance.
  Crescendo, decrescendo, accents on a held chord: all in the hand that is
  not busy with the notes.
* **Even levels:** every note sits exactly where you set it in the mix — no
  accidentally loud note, no note lost because it was played too softly.

Sounds that do not depend on the strength of the attack — organs, pads,
strings, brass and choir ensembles, synth leads — fit this naturally. Sampled
pianos play at one constant dynamic level.

In the future, a foot pedal (expression pedal) per keyboard section may take
over the volume, so both hands stay free for the notes.

For passages that need touch dynamics, connect a velocity-sensitive MIDI
keyboard to the MIDI input of a keyboard section: it plays the same tabs,
with its own velocity. (Analog / Hall-effect gaming keyboards measure the key
travel, but they still appear to the system as ordinary on/off keyboards.)

## Several keyboards, two hands

vkeybd-qt can use several computer keyboards at once, each playing its own
keyboard section with its own tabs and sounds — like the manuals of an organ.
Play one with the left hand and another with the right: accompaniment on
one, melody on the other, each with more than three octaves and its own
octave keys. The notes are arranged like on a chromatic button accordion:
see [The keyboard layout](layout.md).

## N-key rollover (NKRO) — the most important point

When you play chords, hold the sustain (Space) and switch octaves or tabs at
the same time, many keys are down at once. Cheap keyboards can only report a
few of them:

* **Blocking:** to avoid "ghost" key presses, simple keyboards ignore further
  keys once certain combinations are held. The key you press is simply not
  sent — a missing note.
* **6-key rollover (6KRO):** at most six keys plus modifiers at a time.
* **N-key rollover (NKRO):** every key has its own diode, so any number of
  keys in any combination is reported correctly — like on a music keyboard.

Use a keyboard with **full NKRO**.

* **Laptop keyboards** — practically all of them, including MacBooks — have
  no NKRO: they manage only a few keys at once, and some combinations of just
  three keys are blocked already. Some gaming laptops are the exception; check
  their specifications for "NKRO" (not just "anti-ghosting", which often
  means only a few protected keys).
* **Apple keyboards** (Magic Keyboard) are the same: about six keys at most,
  with blocked combinations.
* **Office and membrane keyboards** are usually limited in the same way.

### Check that NKRO is really active

* Many mechanical keyboards ship in 6KRO mode, or switch NKRO on and off with
  an Fn combination. Check the manual.
* Test it: hold as many keys as you can (a big chord plus Space) and watch the
  PC keyboard display of vkeybd-qt — every held key must light up.
* **Linux:** many NKRO keyboards appear as *two* devices — a standard 6KRO one
  and a separate NKRO one. The extra keys may then arrive on a different
  `/dev/input/event*` than the one selected in vkeybd-qt. Check with
  `evtest`: hold seven or more keys and see which device reports them.

## Switches: linear, quiet

* **Linear switches** such as Cherry MX Red (or equivalents): no tactile bump,
  no click, the same force all the way down. Fast repeated notes and runs feel
  even, and nothing in the key fights the finger.
* **Silent linear switches** (e.g. "silent reds") also dampen the bottom-out
  noise — good for quiet stages and anything near a microphone, which would
  otherwise pick up the clacking.
* Avoid clicky (blue) switches: loud, and the click point gets in the way of
  fast playing.

## Keycaps: rounded

Choose keycaps with **rounded edges or spherical tops** instead of the
sharp-edged default caps. During fast runs and glissandi the fingers slide
across keys and rows; sharp edges catch them.

## Lock the keyboard

Give every keyboard section its own physical keyboard and **lock** it, so no
key reaches the operating system or another application during the
performance — no shortcut, no menu, no accidental typing into a window.

* **Linux:** works out of the box (raw keyboard input, *Lock*).
* **Windows:** needs the free [Interception driver](interception.md).
* **macOS:** locking is not possible; the keys always reach the system too.

The details are in the documentation: *Keyboard input and keyboard lock*
(`docs/keyboard-input.md`).

## Checklist before the show

1. NKRO keyboard, NKRO mode on, tested with a big chord plus Space.
2. Every keyboard plugged in before starting vkeybd-qt (Windows with
   Interception: before booting).
3. Each section set to its own keyboard, **locked**.
4. Notifications and screen savers of the computer off.
