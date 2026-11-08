#ifndef PRESET_MODEL_H
#define PRESET_MODEL_H

#include <QString>
#include <QList>

#include <array>

/*
 * Plain data model of one tab ("register" / preset).
 *
 * No QObject, no widgets: this is what the MIDI engine works on and what
 * PresetSerializer reads from / writes to the INI files.
 * Field names and value conventions follow the existing INI keys, so old
 * save files stay compatible.
 */

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
            && this->tremolo == other.tremolo;
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

    TabPreset()
    {
        // same default as the GUI: channel 1 is active
        this->channels[0].activated = true;
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
            && this->resend_midi_auto == other.resend_midi_auto;
    }
    bool operator!=(const TabPreset &other) const { return !(*this == other); }
};

#endif // PRESET_MODEL_H
