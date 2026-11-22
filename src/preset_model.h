#ifndef PRESET_MODEL_H
#define PRESET_MODEL_H

#include <QString>
#include <QList>
#include <QMap>

#include <array>

/*
 * Plain data model of one tab ("register" / preset).
 *
 * No QObject, no widgets: this is what the MIDI engine works on and what
 * PresetSerializer reads from / writes to the INI files.
 * Field names and value conventions follow the existing INI keys, so old
 * save files stay compatible.
 */

// the velocity a channel plays when the notes come from the MIDI input or the
// velocity pedal (PC keys without pedal always use the channel's velocity)
enum class InputVelocity
{
    Fixed = 0,      // the channel's velocity, the played one is ignored
    Scaled = 1,     // played x channel velocity / 64 (64 passes it through)
    AsPlayed = 2    // the played velocity as it is
};

struct ChannelSettings
{
    bool activated = false;
    int volume = 127;
    int pan = 64;
    int key_shift = 0;
    int key_min = 0;
    int key_max = 127;
    int instrument_msb = 0;         // value of the "MSB" spin box (-1 = disabled)
    int instrument_lsb = 0;         // value of the "LSB" spin box (-1 = disabled)
    QString instrument_bank = "Piano";                  // instrument group as shown in the GUI (informational)
    QString instrument_name = "Acoustic Grand Piano";   // instrument as shown in the GUI (informational)
    int velocity = 64;              // MIDI velocity 0..127
    int pitch = 0;                  // static pitch offset -8192..8191
    int portamento_time = 0;
    int attack = 0;
    int release = 0;
    int tremolo = 0;
    
    // MIDI input: which velocity, and which controllers of the input reach this
    // channel (false: the channel keeps its own setting). Pedals and pitch bend always.
    InputVelocity input_velocity = InputVelocity::Scaled;
    bool input_portamento = true;   // CC 5, 65, 84
    bool input_attack = true;       // CC 73
    bool input_release = true;      // CC 72
    bool input_volume = true;       // CC 7
    bool input_pan = true;          // CC 10
    bool input_other = true;        // all other controllers

    // whether a controller of the MIDI input reaches this channel
    bool acceptsInputControl(int cc) const
    {
        switch (cc)
        {
            case 5: case 65: case 84: return this->input_portamento;
            case 73: return this->input_attack;
            case 72: return this->input_release;
            case 7:  return this->input_volume;
            case 10: return this->input_pan;
            default: return this->input_other;
        }
    }

    bool operator==(const ChannelSettings &other) const
    {
        return this->activated == other.activated
            && this->volume == other.volume
            && this->pan == other.pan
            && this->key_shift == other.key_shift
            && this->key_min == other.key_min
            && this->key_max == other.key_max
            && this->instrument_msb == other.instrument_msb
            && this->instrument_lsb == other.instrument_lsb
            && this->instrument_bank == other.instrument_bank
            && this->instrument_name == other.instrument_name
            && this->velocity == other.velocity
            && this->pitch == other.pitch
            && this->portamento_time == other.portamento_time
            && this->attack == other.attack
            && this->release == other.release
            && this->tremolo == other.tremolo
            && this->input_velocity == other.input_velocity
            && this->input_portamento == other.input_portamento
            && this->input_attack == other.input_attack
            && this->input_release == other.input_release
            && this->input_volume == other.input_volume
            && this->input_pan == other.input_pan
            && this->input_other == other.input_other;
    }
    bool operator!=(const ChannelSettings &other) const { return !(*this == other); }
};

struct CCSetting
{
    int channel = 0;                // 0..15
    bool active = true;
    QString label;
    int cc = 0;                     // controller number 0..127
    int value = 0;                  // 0..127
    int delay_ms = 0;               // delay relative to the previous CC entry

    bool operator==(const CCSetting &other) const
    {
        return this->channel == other.channel
            && this->active == other.active
            && this->label == other.label
            && this->cc == other.cc
            && this->value == other.value
            && this->delay_ms == other.delay_ms;
    }
    bool operator!=(const CCSetting &other) const { return !(*this == other); }
};

// how a tab listens to one MIDI input port of its keyboard section
enum class MidiInputMode
{
    WhenSelected = 0,   // only while the tab sounds (current or Menu-selected tab)
    Always = 1          // always, whatever tab is selected (multiplexer: a fixed instrument per input)
};

struct TabPreset
{
    static constexpr int NUMBER_OF_CHANNELS = 16;

    std::array<ChannelSettings, NUMBER_OF_CHANNELS> channels;
    QList<CCSetting> cc_entries;    // in send order

    int master_key_shift = 0;
    int master_volume = 100;        // DCA in percent
    QString octave_mask = "00001000"; // MIDIKeySelector bitmask, char i == '1' -> octave offset 12*i
    int pitchwheel_tether = 64;
    int pitchwheel_pitch = 8192;
    bool resend_midi_auto = false;
    // MIDI input port id of the keyboard section -> mode; empty: no MIDI input plays this tab
    QMap<int, MidiInputMode> midi_inputs;

    // the first input port of every keyboard section; always exists
    static constexpr int DEFAULT_MIDI_INPUT = 0;

    TabPreset()
    {
        // same default as the GUI: channel 1 is active
        this->channels[0].activated = true;
        // as before multiple inputs existed: the section's input plays the sounding tabs
        this->midi_inputs.insert(DEFAULT_MIDI_INPUT, MidiInputMode::WhenSelected);
    }

    bool operator==(const TabPreset &other) const
    {
        return this->channels == other.channels
            && this->cc_entries == other.cc_entries
            && this->master_key_shift == other.master_key_shift
            && this->master_volume == other.master_volume
            && this->octave_mask == other.octave_mask
            && this->pitchwheel_tether == other.pitchwheel_tether
            && this->pitchwheel_pitch == other.pitchwheel_pitch
            && this->resend_midi_auto == other.resend_midi_auto
            && this->midi_inputs == other.midi_inputs;
    }
    bool operator!=(const TabPreset &other) const { return !(*this == other); }
};

#endif // PRESET_MODEL_H
