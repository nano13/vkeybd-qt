# MIDI multiplexer: layers and splits for any MIDI keyboard

You do not have to play on a computer keyboard to get something out of
vkeybd-qt. Every keyboard section has a **MIDI input**: connect a classic
MIDI keyboard, a digital piano, a controller, a sequencer or a DAW track —
and vkeybd-qt turns that one source into layered and split sounds on up to
16 MIDI channels per preset, on as many synths as you like.

```
MIDI keyboard ─► section ─┬─► tab F01-1: channels 1–16 ─► synths
                          └─► tab F03-1: channels 1–16 ─► synths
```

## What it does with the incoming notes

The incoming notes play **all sounding tabs** of the section — the current
tab and every tab checked with right click (or Menu + F-key). Each tab is a
preset of 16 MIDI channels, and every channel can have its own:

* **instrument** — bank and program, sent to the synth,
* **key range** — play only from one note to another: **splits** (bass in
  the left hand, strings in the right) or zones that overlap,
* **key shift** — transpose a layer, e.g. an octave up for a string pad,
* **velocity** — scales the incoming velocity: 64 passes it through
  unchanged, lower values make the layer softer, higher ones louder,
* **volume, pan, attack, release, portamento, tremolo** and further CCs.

So one key on your MIDI keyboard can sound a piano on channel 1, strings an
octave higher on channel 2 and a bass below the split point on channel 3 —
and with a second tab checked, everything of that tab as well.

Each tab has its **own output port**, so the tabs can go to different synths
or plugins, each with its 16 channels.

## Controllers

* **Sustain (CC 64), sostenuto (CC 66) and soft (CC 67)** work like the
  pedals of vkeybd-qt: on all sounding tabs.
* **Pitch bend** and the **other controllers** are forwarded to the sounding
  tabs.
* The notes of all 16 incoming MIDI channels are played (omni).

## Changing presets while you play

Switch between the tabs with the F-keys of the computer keyboard (or a mouse
click), combine them with Menu + F-key, and change their volumes together
with the group DCA — see *Playing* (`docs/playing.md`). Held notes keep sounding
on the tab they were started on while you switch, so the change is seamless.

## Several MIDI sources

Every keyboard section has its own MIDI input (`1-MIDI-in`, `2-MIDI-in`, …)
and its own tabs. Start vkeybd-qt with as many sections as you have sources
(`-n 2`, `-n 3`, …): an upper and a lower manual, a keyboard and a pad
controller, a live player and a sequencer — each with its own layers and
splits, and all of them can still be played from the computer keyboard too.

## Connecting

With ALSA, JACK or libremidi, the MIDI inputs and the tab outputs are
ordinary ports: connect them in your patchbay (qjackctl, Carla, `aconnect`,
the routing of your DAW, …). Details on the outputs: *MIDI output, MIDI input
and tabs* (`docs/midi.md`).
