# MIDI multiplexer: layers and splits for any MIDI keyboard

You do not have to play on a computer keyboard to get something out of
KeyDeskMIDI. Every keyboard section has a **MIDI input**: connect a classic
MIDI keyboard, a digital piano, a controller, a sequencer or a DAW track —
and KeyDeskMIDI turns that one source into layered and split sounds on up to
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
  unchanged, lower values make the layer softer, higher ones louder
  (or, per channel: fixed or as played — see *From Input* below),
* **volume, pan, attack, release, portamento, tremolo** and further CCs.

So one key on your MIDI keyboard can sound a piano on channel 1, strings an
octave higher on channel 2 and a bass below the split point on channel 3 —
and with a second tab checked, everything of that tab as well.

Each tab has its **own output port**, so the tabs can go to different synths
or plugins, each with its 16 channels.

## Controllers

* **Sustain (CC 64), sostenuto (CC 66) and soft (CC 67)** work like the
  pedals of KeyDeskMIDI: on all sounding tabs.
* **Pitch bend** and the **other controllers** are forwarded to the sounding
  tabs.
* The notes of all 16 incoming MIDI channels are played (omni).

### From Input: what a channel takes from the input

In *Edit Channels*, the column **From Input** sets per channel what the MIDI
input (and the velocity pedal) gives it:

* **Velocity** — *scaled* (the default: played × channel velocity / 64),
  *as played*, or *fixed* (the channel's own velocity; the played one is
  ignored — e.g. an organ layer next to a velocity-sensitive piano).
* **Controllers** — portamento (CC 5, 65, 84), attack (CC 73), release
  (CC 72), volume (CC 7), pan (CC 10) and all other controllers can each be
  switched off: the channel then keeps its own setting even if the input
  sends one. The pedals (sustain, sostenuto, soft) and pitch bend always
  reach the channel.

The button shows the velocity mode, and "CC*" when some controllers are
switched off ("no CC" when all are).

## Changing presets while you play

Switch between the tabs with the F-keys of the computer keyboard (or a mouse
click), combine them with Menu + F-key, and change their volumes together
with the group DCA — see *Playing* (`docs/playing.md`). Held notes keep sounding
on the tab they were started on while you switch, so the change is seamless.

## Several MIDI sources

Every keyboard section has its own MIDI input (`1-MIDI-in`, `2-MIDI-in`, …)
and its own tabs. Start KeyDeskMIDI with as many sections as you have sources
(`-n 2`, `-n 3`, …): an upper and a lower manual, a keyboard and a pad
controller, a live player and a sequencer — each with its own layers and
splits, and all of them can still be played from the computer keyboard too.

A section can also have **several input ports**, and every tab chooses which
of them play it: *Tools > MIDI Routing...* (see below). So one section can
take a MIDI keyboard on its first port and a sequencer on a second one that
always plays a fixed tab — whatever you select while you play.

## MIDI routing

*Tools > MIDI Routing...* (with several sections: choose one) shows the input
ports of the section and a matrix of tabs and ports. Each cell is one of:

* **–** — the port does not play this tab,
* **when selected** — the port plays the tab while it sounds: the current tab
  and the tabs selected with Menu + F-key. This follows your stops, like the
  PC keyboard; it is the default for the first port.
* **always** — the port plays the tab whatever is selected: a fixed
  instrument per source, e.g. the drum track of a sequencer.

Several tabs on one port sound together (layers through routing). Pedals
(CC 64, 66, 67), the other controllers and pitch bend of a port go to the
same tabs as its notes.

## Pedals on the input ports

A controller (CC) of a port can get a function instead of being forwarded
to the tabs — for a foot pedal (expression pedal on a MIDI interface, or a
controller with a pedal input):

* **swell** — the volume of the whole keyboard section, on top of all
  channel and tab volumes (and the group DCA): like the swell pedal of an
  organ, which opens and closes the swell box whatever stops are pulled.
  It is never stored in a preset; at start it is fully open.
* **velocity** — the velocity of the notes played on the PC keyboard: touch
  dynamics with the foot. It is scaled by each channel's velocity setting,
  like the velocity of the MIDI input (64 passes it through).

Select the port in *Tools > MIDI Routing...*, press *Learn Pedal* and move
the pedal — or *Add Controller* and set the CC number (11 expression, 4 foot
controller) by hand; then choose the function. A pedal port usually plays no
tab: set all its cells to "–". The controllers are kept with the ports.

Tabs that a port plays **always** have an orange line at the bottom of their
button in the tab switcher.

## Ports

*Add Port* creates a new port (double-click a port to rename it; the first
port can be renamed but not removed). The ports are part of the setup of
your computer and are kept with the keyboard (`keyboard-1.json`, …, next to
`config.ini`); which port plays which tab is part of the tabs and is saved
with them (*File > Save*, *QuickSave*). Renaming keeps the patchbay
connections with ALSA and JACK; removing a port releases its held notes.

## Connecting

With ALSA, JACK or libremidi, the MIDI inputs and the tab outputs are
ordinary ports: connect them in your patchbay (qjackctl, Carla, `aconnect`,
the routing of your DAW, …). Details on the outputs: *MIDI output, MIDI input
and tabs* (`docs/midi.md`).
