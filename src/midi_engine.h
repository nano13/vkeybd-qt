#ifndef MIDI_ENGINE_H
#define MIDI_ENGINE_H

#include <QObject>
#include <QHash>
#include <QList>

#include "preset_model.h"
#include "interface_audio.h"

/*
 * Core MIDI engine of one keyboard instance (one MainTabs).
 *
 * - owns one TabPreset per tab: this is the single source of truth for the
 *   channel setup. The GUI (channel editor, summaries) only reads it or calls
 *   the setters below.
 * - keeps a "compiled" routing table per tab, so a key press does not have to
 *   read any widget.
 * - remembers exactly which notes were sent for which source key. A key release
 *   always releases those notes, no matter if the key shift, the active tab,
 *   the octave selection or the channel setup changed in the meantime.
 * - sends all channel related MIDI (controls, program changes, CCs, pedals).
 *
 * The MIDI port of a tab is its tab id. Source keys are the codes emitted by
 * KeyboardPC (MIDIPress / MIDIRelease).
 */
class MidiEngine : public QObject
{
    Q_OBJECT
public:
    explicit MidiEngine(InterfaceAudio *interface_audio, QObject *parent = nullptr);

    // The lower full octave of the PC keyboard is in the range 12..23 (to be able
    // to add some even deeper keys to the left), therefore the offset of 12.
    static constexpr int SOURCE_KEY_OFFSET = 12;
    // MIDI input, see midiNoteOn()
    static constexpr int MIDI_INPUT_REFERENCE_OCTAVE = 48;
    static constexpr int MIDI_INPUT_NEUTRAL_VELOCITY = 64;
    // held notes of the MIDI input are kept apart from the PC keys (8..48)
    static constexpr int MIDI_INPUT_HELD_KEY_BASE = 1000;
    // range of the key shift spin boxes
    static constexpr int KEY_SHIFT_LIMIT = 64;

    // ---- preset ----
    // replaces everything, sends nothing (use resendControls() afterwards if wanted)
    void setPreset(int tab, const TabPreset &preset);
    // takes over everything except channels and CC entries, sends nothing
    void setPresetMain(int tab, const TabPreset &main_source);
    // sends the MIDI controls that differ from the previous channel state
    void setChannel(int tab, int channel, const ChannelSettings &settings);
    // stores only; CC values are sent with sendControlChange() / resendControls()
    void setCCEntries(int tab, const QList<CCSetting> &cc_entries);
    // DCA in percent, re-sends the volume of all channels
    void setMasterVolume(int tab, int percent);
    
    // set by the user: also becomes the value a relative global reset returns to
    void setMasterKeyShift(int tab, int value);
    // global key shift for all tabs:
    //  absolute: every tab gets value
    //  relative: value is added; value 0 resets every tab to its last user-set key shift
    void globalKeyShift(int value, bool is_relative);
    QList<int> tabs() const;

    bool hasPreset(int tab) const;
    TabPreset preset(int tab) const;

    // ---- notes ----
    void noteOn(int tab, int source_key);
    void noteOff(int source_key);
    
    // MIDI input: notes are routed like PC keys (key ranges, shifts, octave checkboxes;
    // the default octave 48 plays the note as it is). They are held separately from
    // the PC keys. The velocity is scaled by the channel's velocity setting relative
    // to mf (64), so the default setting passes it through unchanged.
    void midiNoteOn(int tab, int note, int velocity);
    void midiNoteOff(int note);
    // sends a CC to all activated channels of a tab
    void forwardControlChange(int tab, int cc, int value);
    void pitchBend(int tab, int value);

    // sends note-off for everything still held
    void releaseAll();
    // drops the bookkeeping of a tab without sending anything (after all-notes-off)
    void forgetTab(int tab);
    int heldNoteCount() const;

    // ---- controls ----
    void resendControls(int tab);
    void sendControlChange(int tab, int channel, int cc, int value);
    void panic(int tab);
    void stopAll(int tab);
    void sustain(int tab, bool pressed);
    void sostenuto(int tab, bool pressed);
    void soft(int tab, bool pressed);

signals:
    void signalNoteOn(int tab, int note);
    void signalNoteOff(int tab, int note);
    void signalSourceKey(int tab, int source_key, bool pressed);
    // emitted by setPreset(): the whole preset of a tab was replaced
    void signalPresetReplaced(int tab);

private:
    struct NoteRoute
    {
        int channel;
        int key_shift;
        int key_min;
        int key_max;
        int velocity;
    };

    struct CompiledTab
    {
        QList<int> octave_offsets;
        QList<NoteRoute> routes;        // one per activated channel
        int master_key_shift = 0;
    };

    struct SentNote
    {
        int tab;
        int channel;
        int note;
        int velocity;
    };

    static CompiledTab compile(const TabPreset &preset);
    void ensureTab(int tab);
    void recompile(int tab);

    // velocity < 0: use the channel's velocity setting
    void startNotes(int tab, int held_key, int keycode, int velocity);
    
    // only_tab < 0: release for all tabs
    void releaseHeld(int source_key, int only_tab);

    void sendProgram(int tab, int channel, const ChannelSettings &settings);
    void sendVolume(int tab, int channel, const ChannelSettings &settings, int master_volume);

    InterfaceAudio *interface_audio;

    QHash<int, TabPreset> presets;
    QHash<int, CompiledTab> compiled;
    QHash<int, int> master_key_shift_start;     // runtime only, not saved

    QHash<int, QList<SentNote>> held_notes;     // source key -> notes actually sent
    QHash<int, QList<int>> held_source_tabs;    // source key -> tabs that saw the press (for the displays)
};

#endif // MIDI_ENGINE_H
