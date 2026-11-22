#include "midi_engine.h"

#include <QTimer>

#include <algorithm>

MidiEngine::MidiEngine(InterfaceAudio *interface_audio, QObject *parent)
    : QObject(parent), interface_audio(interface_audio)
{

}

void MidiEngine::setPreset(int tab, const TabPreset &preset)
{
    this->presets.insert(tab, preset);
    recompile(tab);
    
    emit signalPresetReplaced(tab);
}

void MidiEngine::setPresetMain(int tab, const TabPreset &main_source)
{
    ensureTab(tab);
    
    TabPreset &preset = this->presets[tab];
    preset.master_key_shift = main_source.master_key_shift;
    preset.master_volume = main_source.master_volume;
    preset.octave_mask = main_source.octave_mask;
    preset.pitchwheel_tether = main_source.pitchwheel_tether;
    preset.pitchwheel_pitch = main_source.pitchwheel_pitch;
    preset.resend_midi_auto = main_source.resend_midi_auto;
    
    recompile(tab);
}

void MidiEngine::setChannel(int tab, int channel, const ChannelSettings &settings)
{
    if (channel < 0 || channel >= TabPreset::NUMBER_OF_CHANNELS)
        return;
    
    ensureTab(tab);
    
    const ChannelSettings old = this->presets.value(tab).channels.at(channel);
    this->presets[tab].channels[channel] = settings;
    const int master_volume = this->presets.value(tab).master_volume;
    
    recompile(tab);
    
    if (!this->interface_audio)
        return;
    
    // only what actually changed goes out
    if (old.instrument_msb != settings.instrument_msb || old.instrument_lsb != settings.instrument_lsb)
        sendProgram(tab, channel, settings);
    // an activated channel may have missed volume changes (swell) while it was off
    if (old.volume != settings.volume || (settings.activated && !old.activated))
        sendVolume(tab, channel, settings, master_volume);
    if (old.pan != settings.pan)
        this->interface_audio->setPanChangeEvent(tab, channel, static_cast<uint8_t>(qBound(0, settings.pan, 127)));
    if (old.portamento_time != settings.portamento_time)
        this->interface_audio->setPortamentoChanged(tab, channel, static_cast<uint8_t>(qBound(0, settings.portamento_time, 127)));
    if (old.attack != settings.attack)
        this->interface_audio->setAttackChanged(tab, channel, static_cast<uint8_t>(qBound(0, settings.attack, 127)));
    if (old.release != settings.release)
        this->interface_audio->setReleaseChanged(tab, channel, static_cast<uint8_t>(qBound(0, settings.release, 127)));
    if (old.tremolo != settings.tremolo)
        this->interface_audio->setTremoloChanged(tab, channel, static_cast<uint8_t>(qBound(0, settings.tremolo, 127)));
    if (old.pitch != settings.pitch)
        this->interface_audio->keyPitchbendEvent(tab, channel, qBound(0, settings.pitch + 8192, 16383));
}

void MidiEngine::setCCEntries(int tab, const QList<CCSetting> &cc_entries)
{
    ensureTab(tab);
    this->presets[tab].cc_entries = cc_entries;
}

void MidiEngine::setMasterVolume(int tab, int percent)
{
    ensureTab(tab);
    
    if (this->presets.value(tab).master_volume == percent)
        return;
    
    this->presets[tab].master_volume = percent;
    
    // all channels, also deactivated ones: they may get activated later without a resend
    const TabPreset preset = this->presets.value(tab);
    for (int c = 0; c < TabPreset::NUMBER_OF_CHANNELS; c++)
    {
        sendVolume(tab, c, preset.channels.at(c), percent);
    }
}

void MidiEngine::setSwell(int value)
{
    value = qBound(0, value, 127);
    if (value == this->swell_value)
        return;
    this->swell_value = value;
    
    // the activated channels; the others get their volume when they are activated
    const QList<int> tabs = this->presets.keys();
    for (const int tab : tabs)
    {
        const TabPreset preset = this->presets.value(tab);
        for (int c = 0; c < TabPreset::NUMBER_OF_CHANNELS; c++)
        {
            if (preset.channels.at(c).activated)
                sendVolume(tab, c, preset.channels.at(c), preset.master_volume);
        }
    }
}

int MidiEngine::swell() const
{
    return this->swell_value;
}

void MidiEngine::setMasterKeyShift(int tab, int value)
{
    ensureTab(tab);
    
    value = qBound(-KEY_SHIFT_LIMIT, value, KEY_SHIFT_LIMIT);
    this->presets[tab].master_key_shift = value;
    
    recompile(tab);
}

void MidiEngine::setGlobalKeyShift(int value)
{
    // applies to the next notes; held notes keep sounding as they were started
    value = qBound(-KEY_SHIFT_LIMIT, value, KEY_SHIFT_LIMIT);
    if (value == this->global_key_shift)
        return;
    this->global_key_shift = value;
    emit signalRoutingChanged();
}

int MidiEngine::globalKeyShift() const
{
    return this->global_key_shift;
}

QList<int> MidiEngine::tabs() const
{
    QList<int> result = this->presets.keys();
    std::sort(result.begin(), result.end());
    return result;
}

void MidiEngine::ensureTab(int tab)
{
    if (!this->presets.contains(tab))
    {
        this->presets.insert(tab, TabPreset());
        recompile(tab);
    }
}

void MidiEngine::recompile(int tab)
{
    this->compiled.insert(tab, compile(this->presets.value(tab)));
    emit signalRoutingChanged();
}

bool MidiEngine::hasPreset(int tab) const
{
    return this->presets.contains(tab);
}

TabPreset MidiEngine::preset(int tab) const
{
    return this->presets.value(tab);
}

MidiEngine::CompiledTab MidiEngine::compile(const TabPreset &preset)
{
    CompiledTab result;
    result.master_key_shift = preset.master_key_shift;

    for (int i = 0; i < preset.octave_mask.length(); i++)
    {
        if (preset.octave_mask.at(i) == QLatin1Char('1'))
        {
            result.octave_offsets.append(12 * i);
        }
    }

    for (int c = 0; c < TabPreset::NUMBER_OF_CHANNELS; c++)
    {
        const ChannelSettings &channel = preset.channels.at(c);
        if (!channel.activated)
            continue;

        NoteRoute route;
        route.channel = c;
        route.key_shift = channel.key_shift;
        route.key_min = channel.key_min;
        route.key_max = channel.key_max;
        route.velocity = qBound(0, channel.velocity, 127);
        route.input_velocity = channel.input_velocity;

        result.routes.append(route);
    }

    return result;
}

int MidiEngine::displayNoteForSourceKey(int source_key)
{
    if (source_key >= MIDI_INPUT_HELD_KEY_BASE)
        return (source_key - MIDI_INPUT_HELD_KEY_BASE) % 128 - MIDI_INPUT_REFERENCE_OCTAVE;
    return source_key;
}

void MidiEngine::noteOn(int tab, int source_key, int velocity)
{
    startNotes(tab, source_key, source_key - SOURCE_KEY_OFFSET, velocity);
}

void MidiEngine::midiNoteOn(int tab, int input, int note, int velocity)
{
    // the default octave checkbox (48) plays the incoming note as it is
    startNotes(tab, MIDI_INPUT_HELD_KEY_BASE + input * 128 + note, note - MIDI_INPUT_REFERENCE_OCTAVE, velocity);
}

void MidiEngine::midiNoteOff(int input, int note)
{
    releaseHeld(MIDI_INPUT_HELD_KEY_BASE + input * 128 + note, -1);
}

void MidiEngine::setMidiInputs(int tab, const QMap<int, MidiInputMode> &inputs)
{
    ensureTab(tab);
    this->presets[tab].midi_inputs = inputs;
}

void MidiEngine::forwardControlChange(int tab, int cc, int value)
{
    if (!this->compiled.contains(tab))
        return;
    
    const CompiledTab compiled_tab = this->compiled.value(tab);
    const TabPreset preset = this->presets.value(tab);
    for (const NoteRoute &route : compiled_tab.routes)
    {
        // a channel can keep its own portamento, attack, ... (Edit Channels: From Input)
        if (preset.channels.at(route.channel).acceptsInputControl(cc))
            sendControlChange(tab, route.channel, cc, value);
    }
}

QList<MidiEngine::RoutedNote> MidiEngine::routeKey(int tab, int keycode) const
{
    QList<RoutedNote> result;
    if (!this->compiled.contains(tab))
        return result;
    
    const CompiledTab compiled_tab = this->compiled.value(tab);
    for (int octave_offset : compiled_tab.octave_offsets)
    {
        const int m_code = keycode + octave_offset;
        
        for (const NoteRoute &route : compiled_tab.routes)
        {
            if (m_code < route.key_min || m_code > route.key_max)
                continue;
            
            const int note = m_code + route.key_shift + compiled_tab.master_key_shift + this->global_key_shift;
            if (note < 0 || note > 127)
                continue;
            
            RoutedNote routed;
            routed.channel = route.channel;
            routed.note = note;
            routed.velocity = route.velocity;
            routed.input_velocity = route.input_velocity;
            result.append(routed);
        }
    }
    return result;
}

QList<int> MidiEngine::soundingNotes(int tab, int source_key) const
{
    QList<int> notes;
    const QList<RoutedNote> routed = routeKey(tab, source_key - SOURCE_KEY_OFFSET);
    for (const RoutedNote &r : routed)
    {
        if (!notes.contains(r.note))
            notes.append(r.note);
    }
    std::sort(notes.begin(), notes.end());
    return notes;
}

void MidiEngine::startNotes(int tab, int held_key, int keycode, int velocity)
{
    if (!this->compiled.contains(tab))
        return;
    
    // a second press without release (e.g. lost key-up while the window had no focus):
    // release what this tab still holds for this key before re-triggering
    releaseHeld(held_key, tab);
    
    QList<SentNote> sent_notes;
    const QList<RoutedNote> routed = routeKey(tab, keycode);
    for (const RoutedNote &r : routed)
    {
        SentNote sent;
        sent.tab = tab;
        sent.channel = r.channel;
        sent.note = r.note;
        if (velocity < 0 || r.input_velocity == InputVelocity::Fixed)
        {
            // PC keyboard (no velocity pedal), or the channel ignores the played velocity
            sent.velocity = r.velocity;
        }
        else if (r.input_velocity == InputVelocity::AsPlayed)
        {
            sent.velocity = qBound(1, velocity, 127);
        }
        else
        {
            // MIDI input / velocity pedal: scaled by the channel setting relative to mf (64)
            sent.velocity = qBound(1, velocity * r.velocity / MIDI_INPUT_NEUTRAL_VELOCITY, 127);
        }
        sent_notes.append(sent);
    }
    
    // update the bookkeeping first, then talk to the outside world
    if (!sent_notes.isEmpty())
    {
        this->held_notes[held_key].append(sent_notes);
    }
    if (!this->held_source_tabs[held_key].contains(tab))
    {
        this->held_source_tabs[held_key].append(tab);
    }
    
    for (const SentNote &sent : sent_notes)
    {
        if (this->interface_audio)
        {
            this->interface_audio->keyPressEvent(sent.tab, sent.channel, static_cast<uint8_t>(sent.note), static_cast<uint8_t>(sent.velocity));
        }
        emit signalNoteOn(sent.tab, sent.note);
    }
    
    emit signalSourceKey(tab, held_key, true);
}

void MidiEngine::noteOff(int source_key)
{
    releaseHeld(source_key, -1);
}

void MidiEngine::releaseHeld(int source_key, int only_tab)
{
    QList<SentNote> to_release;
    if (this->held_notes.contains(source_key))
    {
        QList<SentNote> all = this->held_notes.take(source_key);
        QList<SentNote> keep;
        for (const SentNote &sent : all)
        {
            if (only_tab < 0 || sent.tab == only_tab)
                to_release.append(sent);
            else
                keep.append(sent);
        }
        if (!keep.isEmpty())
        {
            this->held_notes.insert(source_key, keep);
        }
    }

    QList<int> released_tabs;
    if (this->held_source_tabs.contains(source_key))
    {
        QList<int> tabs = this->held_source_tabs.take(source_key);
        QList<int> keep_tabs;
        for (int tab : tabs)
        {
            if (only_tab < 0 || tab == only_tab)
                released_tabs.append(tab);
            else
                keep_tabs.append(tab);
        }
        if (!keep_tabs.isEmpty())
        {
            this->held_source_tabs.insert(source_key, keep_tabs);
        }
    }

    // bookkeeping is consistent now; slots connected to the signals may call back into the engine
    for (const SentNote &sent : to_release)
    {
        if (this->interface_audio)
        {
            this->interface_audio->keyReleaseEvent(sent.tab, sent.channel, static_cast<uint8_t>(sent.note), static_cast<uint8_t>(sent.velocity));
        }
        emit signalNoteOff(sent.tab, sent.note);
    }
    for (int tab : released_tabs)
    {
        emit signalSourceKey(tab, source_key, false);
    }
}

void MidiEngine::pitchBend(int tab, int value)
{
    if (!this->compiled.contains(tab) || !this->interface_audio)
        return;

    const CompiledTab compiled_tab = this->compiled.value(tab);

    // once per activated channel (and not once per selected octave)
    for (const NoteRoute &route : compiled_tab.routes)
    {
        this->interface_audio->keyPitchbendEvent(tab, route.channel, value);
    }
}

void MidiEngine::releaseAll()
{
    QList<int> keys = this->held_source_tabs.keys();
    QList<int> note_keys = this->held_notes.keys();
    for (int key : note_keys)
    {
        if (!keys.contains(key))
            keys.append(key);
    }

    for (int key : keys)
    {
        releaseHeld(key, -1);
    }
}

void MidiEngine::forgetTab(int tab)
{
    QList<int> note_keys = this->held_notes.keys();
    for (int key : note_keys)
    {
        QList<SentNote> keep;
        QList<SentNote> all = this->held_notes.value(key);
        for (const SentNote &sent : all)
        {
            if (sent.tab != tab)
                keep.append(sent);
        }

        if (keep.isEmpty())
            this->held_notes.remove(key);
        else
            this->held_notes.insert(key, keep);
    }

    QList<int> tab_keys = this->held_source_tabs.keys();
    for (int key : tab_keys)
    {
        QList<int> tabs = this->held_source_tabs.value(key);
        tabs.removeAll(tab);

        if (tabs.isEmpty())
            this->held_source_tabs.remove(key);
        else
            this->held_source_tabs.insert(key, tabs);
    }
}

int MidiEngine::heldNoteCount() const
{
    int count = 0;
    QList<QList<SentNote>> lists = this->held_notes.values();
    for (const QList<SentNote> &list : lists)
    {
        count += list.length();
    }
    return count;
}

void MidiEngine::sendProgram(int tab, int channel, const ChannelSettings &settings)
{
    // -1 means "[disabled]": do not touch the instrument on the synth
    if (!this->interface_audio || settings.instrument_msb < 0 || settings.instrument_lsb < 0)
        return;
    
    this->interface_audio->setProgramChangeEvent(tab, channel, static_cast<uint8_t>(qBound(0, settings.instrument_msb, 127)), settings.instrument_lsb);
}

void MidiEngine::sendVolume(int tab, int channel, const ChannelSettings &settings, int master_volume)
{
    if (!this->interface_audio)
        return;
    
    // channel x tab (DCA) x swell pedal
    int volume = settings.volume * master_volume * this->swell_value / (100 * 127);
    this->interface_audio->setVolumeChangeEvent(tab, channel, static_cast<uint8_t>(qBound(0, volume, 127)));
}

void MidiEngine::resendControls(int tab)
{
    if (!this->presets.contains(tab) || !this->interface_audio)
        return;
    
    const TabPreset preset = this->presets.value(tab);
    
    for (int c = 0; c < TabPreset::NUMBER_OF_CHANNELS; c++)
    {
        const ChannelSettings &settings = preset.channels.at(c);
        if (!settings.activated)
            continue;
        
        // program change first, everything else after it
        sendProgram(tab, c, settings);
        sendVolume(tab, c, settings, preset.master_volume);
        this->interface_audio->setPanChangeEvent(tab, c, static_cast<uint8_t>(qBound(0, settings.pan, 127)));
        this->interface_audio->setPortamentoChanged(tab, c, static_cast<uint8_t>(qBound(0, settings.portamento_time, 127)));
        this->interface_audio->setAttackChanged(tab, c, static_cast<uint8_t>(qBound(0, settings.attack, 127)));
        this->interface_audio->setReleaseChanged(tab, c, static_cast<uint8_t>(qBound(0, settings.release, 127)));
        this->interface_audio->setTremoloChanged(tab, c, static_cast<uint8_t>(qBound(0, settings.tremolo, 127)));
        this->interface_audio->keyPitchbendEvent(tab, c, qBound(0, settings.pitch + 8192, 16383));
    }
    
    // active CC entries in their order, each delayed relative to the previous one
    int cumulative_delay = 0;
    for (const CCSetting &cc : preset.cc_entries)
    {
        if (!cc.active)
            continue;
        
        cumulative_delay += cc.delay_ms;
        
        const int channel = cc.channel;
        const int controller = cc.cc;
        const int value = cc.value;
        if (cumulative_delay == 0)
        {
            sendControlChange(tab, channel, controller, value);
        }
        else
        {
            QTimer::singleShot(cumulative_delay, this, [this, tab, channel, controller, value]() {
                sendControlChange(tab, channel, controller, value);
            });
        }
    }
}

void MidiEngine::sendControlChange(int tab, int channel, int cc, int value)
{
    if (!this->interface_audio)
        return;
    
    this->interface_audio->setControlChangeEvent(tab, channel, static_cast<uint8_t>(qBound(0, cc, 127)), static_cast<uint8_t>(qBound(0, value, 127)));
}

void MidiEngine::panic(int tab)
{
    forgetTab(tab);
    
    if (!this->presets.contains(tab) || !this->interface_audio)
        return;
    
    const CompiledTab compiled_tab = this->compiled.value(tab);
    for (const NoteRoute &route : compiled_tab.routes)
    {
        this->interface_audio->keyPanicEvent(tab, route.channel);
    }
}

void MidiEngine::stopAll(int tab)
{
    forgetTab(tab);
    
    if (!this->presets.contains(tab) || !this->interface_audio)
        return;
    
    const CompiledTab compiled_tab = this->compiled.value(tab);
    for (const NoteRoute &route : compiled_tab.routes)
    {
        this->interface_audio->keyStopAllEvent(tab, route.channel);
    }
}

void MidiEngine::sustain(int tab, bool pressed)
{
    if (!this->presets.contains(tab) || !this->interface_audio)
        return;
    
    const CompiledTab compiled_tab = this->compiled.value(tab);
    for (const NoteRoute &route : compiled_tab.routes)
    {
        this->interface_audio->keySustainEvent(tab, route.channel, pressed);
    }
}

void MidiEngine::sostenuto(int tab, bool pressed)
{
    if (!this->presets.contains(tab) || !this->interface_audio)
        return;
    
    const CompiledTab compiled_tab = this->compiled.value(tab);
    for (const NoteRoute &route : compiled_tab.routes)
    {
        this->interface_audio->keySostenutoEvent(tab, route.channel, pressed);
    }
}

void MidiEngine::soft(int tab, bool pressed)
{
    if (!this->presets.contains(tab) || !this->interface_audio)
        return;
    
    const CompiledTab compiled_tab = this->compiled.value(tab);
    for (const NoteRoute &route : compiled_tab.routes)
    {
        this->interface_audio->keySoftEvent(tab, route.channel, pressed);
    }
}
