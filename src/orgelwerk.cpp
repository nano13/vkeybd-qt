#include "orgelwerk.h"

Orgelwerk::Orgelwerk(int keyboard_id, MidiEngine *engine, QWidget *parent) : QWidget(parent)
{
    this->keyboard_id = keyboard_id;
    this->engine = engine;

    drawGUI();

    // the engine reports what was actually sent, the displays follow that
    connect(this->engine, &MidiEngine::signalNoteOn, this, &Orgelwerk::engineNoteOn);
    connect(this->engine, &MidiEngine::signalNoteOff, this, &Orgelwerk::engineNoteOff);
    connect(this->engine, &MidiEngine::signalSourceKey, this, &Orgelwerk::engineSourceKey);
    connect(this->engine, &MidiEngine::signalPresetReplaced, this, &Orgelwerk::enginePresetReplaced);

    // every change of the settings shown here goes straight to the engine
    connect(this->keys, &MIDIKeySelector::signalSelectionChanged, this, &Orgelwerk::pushMain);
    connect(this->key_shift_master, &MIDIKeyShiftWidget::signalValueChanged, this, &Orgelwerk::keyShiftChanged);
    connect(this->pitch, &MIDIPitchWheel::tetherChanged, this, &Orgelwerk::pushMain);
    connect(this->check_resend_midi_auto, &QCheckBox::toggled, this, &Orgelwerk::pushMain);

    installEventFilter(this);
}

void Orgelwerk::drawGUI()
{
    this->keys = new MIDIKeySelector;
    this->pitch = new MIDIPitchWheel(this);
    this->volume = new MIDIMasterVolume(this);
    this->key_shift_master = new MIDIKeyShiftWidget;

    connect(this->pitch, &MIDIPitchWheel::pitchWheelMoved, this, &Orgelwerk::pitchWheelMoved);
    connect(this->volume, &MIDIMasterVolume::sliderMoved, this, &Orgelwerk::volumeSliderMoved);

    this->grid = new QGridLayout;
    setLayout(this->grid);

    this->group_keys = new QGroupBox("Keys");
    this->group_pitch = new QGroupBox("Pitch && Volume");

    QVBoxLayout *layout_keys = new QVBoxLayout;
    QGridLayout *layout_pitch = new QGridLayout;

    this->group_keys->setLayout(layout_keys);
    this->group_pitch->setLayout(layout_pitch);

    this->group_keys->setMaximumHeight(60);

    QLabel *label_key_shift_master = new QLabel("Key Shift");

    layout_keys->addWidget(this->keys);
    QGridLayout *layout_pitch_volume = new QGridLayout;
    layout_pitch_volume->addWidget(label_key_shift_master, 0, 0);
    layout_pitch_volume->addWidget(this->key_shift_master, 1, 0);
    layout_pitch_volume->addWidget(this->volume->label_volume, 0, 1);
    layout_pitch_volume->addWidget(this->volume->slider_volume, 1, 1);

    layout_pitch->addLayout(layout_pitch_volume, 0, 0, 1, 2);
    layout_pitch->addWidget(this->pitch->label_tether, 1, 0);
    layout_pitch->addWidget(this->pitch->slider_tether, 2, 0);

    layout_pitch->addWidget(this->volume->label_tether, 1, 1);
    layout_pitch->addWidget(this->volume->slider_tether, 2, 1);

    layout_pitch->addWidget(this->pitch->label_pitch, 3, 0);
    layout_pitch->addWidget(this->pitch->slider_pitch, 4, 0, 1, 2);

    this->button_panic->setText("Panic! [Esc]");
    connect(this->button_panic, &QPushButton::clicked, this, &Orgelwerk::panicPressed);
    this->button_panic->setObjectName("button_panic");

    this->button_stop_all->setText("Stop All [Del]");
    connect(this->button_stop_all, &QPushButton::clicked, this, &Orgelwerk::stopAllPressed);
    this->button_stop_all->setObjectName("button_stop");

    QLayout *layout_panic_stop = new QHBoxLayout;
    layout_panic_stop->addWidget(this->button_panic);
    layout_panic_stop->addWidget(this->button_stop_all);

    showChannelsSummary(1);
    this->grid->addWidget(this->group_keys, 4, 0, 1, 2);
    this->grid->addWidget(this->group_pitch, 5, 0, 1, 2);
    drawNotesKeyboard(6);
    drawPCKeyboard(8);
    drawPianoKeyboard(7);
    this->grid->addLayout(layout_panic_stop, 10, 0, 1, 2);
}

void Orgelwerk::drawNotesKeyboard(int grid_row)
{
    this->notes = new KeyboardNotes;
    this->notes->setToolTip("Shows which notes are actually played");

    this->grid->addWidget(this->notes, grid_row, 0, 1, 2);
}
void Orgelwerk::drawPianoKeyboard(int grid_row)
{
    this->piano = new KeyboardPiano;
    this->piano->setMidiNotes(this->pc->midiNotes());
    connect(this->pc, &KeyboardPC::midiMappingChanged, this->piano, [this]{
        this->piano->setMidiNotes(this->pc->midiNotes());
        // The piano rebuilds its buttons when the PC mapping changes.
        for (QHash<int, int>::const_iterator it = this->displayed_note_counts.cbegin();
             it != this->displayed_note_counts.cend(); ++it)
        {
            if (it.value() > 0)
                this->piano->keyPressed(it.key());
        }
    });

    // Expand to the same column width as the PC keyboard below.
    this->grid->addWidget(this->piano, grid_row, 0, 1, 2);
    this->piano->hide();
}
void Orgelwerk::drawPCKeyboard(int grid_row)
{
    this->pc = new KeyboardPC;
    this->pc->setToolTip("Shows what keys are pressed");
    connect(this->pc, &KeyboardPC::MIDIPress, this, [this](int midicode){ emit signalMIDIEvent(midicode, MIDIMode::Press); });
    connect(this->pc, &KeyboardPC::MIDIRelease, this, [this](int midicode){ emit signalMIDIEvent(midicode, MIDIMode::Release); });
    connect(this->pc, &KeyboardPC::octaveDown, this, &Orgelwerk::octaveDown);
    connect(this->pc, &KeyboardPC::octaveUp, this, &Orgelwerk::octaveUp);

    // The key-press display expands to the full width of this keyboard column.
    this->grid->addWidget(this->pc, grid_row, 0, 1, 2);
}

void Orgelwerk::showChannelsSummary(int grid_row)
{
    QGroupBox *group_channels = new QGroupBox("Channels");
    QGridLayout *layout_channels = new QGridLayout;
    group_channels->setLayout(layout_channels);

    this->midi_channels_summary = new MIDIChannelsSummary;

    this->button_channels_dialog->setText("Edit Channels");
    connect(this->button_channels_dialog, &QPushButton::clicked, this, [this]{ emit signalEditChannels(this->tab_id); });

    this->button_resend_midi->setText("Resend Settings");
    this->button_resend_midi->setToolTip("Resend MIDI Settings [Ins]");
    connect(this->button_resend_midi, &QPushButton::clicked, this, &Orgelwerk::resendMIDIControls);

    this->check_resend_midi_auto->setToolTip("Automatically resend MIDI settings if tab activated");

    QPushButton *button_keymap_tab = new QPushButton("Keymap");

    layout_channels->setVerticalSpacing(0);
    layout_channels->addWidget(this->midi_channels_summary, 0, 0, 1, 4);
    layout_channels->addWidget(this->button_channels_dialog, 1, 0);
    layout_channels->addWidget(this->button_resend_midi, 1, 1);
    layout_channels->addWidget(this->check_resend_midi_auto, 1, 2);
    layout_channels->addWidget(button_keymap_tab, 1, 3);

    this->grid->addWidget(group_channels, grid_row, 0, 1, 2);
}

// ---------------------------------------------------------------------------
// binding

void Orgelwerk::bindTab(int tab)
{
    if (tab == this->tab_id)
        return;

    // the pitch wheel position of the previous tab is only written back now
    storeToEngine();

    this->tab_id = tab;
    
    // the offsets were relative to the previous tab
    this->follow_key_shift.clear();

    resetNoteDisplays();
    reloadFromEngine();
}

int Orgelwerk::boundTab() const
{
    return this->tab_id;
}

void Orgelwerk::setSoundingTabs(std::function<QList<int>()> provider)
{
    this->sounding_tabs = provider;
}

QList<int> Orgelwerk::soundingTabs() const
{
    QList<int> tabs;
    if (this->tab_id < 0)
        return tabs;
    
    tabs.append(this->tab_id);
    if (this->sounding_tabs)
    {
        const QList<int> provided = this->sounding_tabs();
        for (const int tab : provided)
        {
            if (tab >= 0 && !tabs.contains(tab))
                tabs.append(tab);
        }
    }
    return tabs;
}

void Orgelwerk::refreshVolume()
{
    if (this->tab_id >= 0)
        this->volume->setValueSilently(this->engine->preset(this->tab_id).master_volume);
}

void Orgelwerk::followKeyShift(int delta)
{
    if (delta == 0)
        return;
    
    QList<int> others = soundingTabs();
    others.removeAll(this->tab_id);
    
    const int minimum = -MidiEngine::KEY_SHIFT_LIMIT;
    const int maximum = MidiEngine::KEY_SHIFT_LIMIT;
    
    // a tab that is not checked anymore starts over next time
    const QList<int> followed = this->follow_key_shift.keys();
    for (const int tab : followed)
    {
        if (!others.contains(tab))
            this->follow_key_shift.remove(tab);
    }
    
    for (const int tab : others)
    {
        const int actual = this->engine->preset(tab).master_key_shift;
        
        int value = this->follow_key_shift.value(tab, actual);
        // changed elsewhere in the meantime (e.g. on its own tab): continue from there
        if (qBound(minimum, value, maximum) != actual)
            value = actual;
        
        value += delta;
        this->follow_key_shift.insert(tab, value);
        this->engine->setMasterKeyShift(tab, qBound(minimum, value, maximum));
    }
}

QList<int> Orgelwerk::pedalTabs(bool pressed, QSet<int> &held) const
{
    QList<int> tabs = soundingTabs();
    if (pressed)
    {
        for (const int tab : tabs)
            held.insert(tab);
    }
    else
    {
        for (const int tab : held)
        {
            if (!tabs.contains(tab))
                tabs.append(tab);
        }
        held.clear();
    }
    return tabs;
}

void Orgelwerk::reloadFromEngine()
{
    if (this->tab_id < 0)
        return;

    const TabPreset preset = this->engine->preset(this->tab_id);

    this->loading = true;

    this->key_shift_master->setValueFromGlobal(preset.master_key_shift);
    this->volume->setValueSilently(preset.master_volume);
    this->keys->restoreBitmaskOfKeys(preset.octave_mask);

    QMap<QString,QVariant> pitchwheel;
    pitchwheel["pitchwheel_tether"] = preset.pitchwheel_tether;
    pitchwheel["pitchwheel_pitch"] = preset.pitchwheel_pitch;
    this->pitch->setData(pitchwheel);

    this->check_resend_midi_auto->setChecked(preset.resend_midi_auto);

    this->loading = false;

    channelsSummaryUpdate();
}

void Orgelwerk::storeToEngine()
{
    pushMain();
}

void Orgelwerk::pushMain()
{
    if (this->loading || this->tab_id < 0)
        return;

    TabPreset main = PresetSerializer::fromGuiMaps(
                QList<QMap<QString,QVariant>>(),
                getParams(),
                this->check_resend_midi_auto->isChecked()
                );
    this->engine->setPresetMain(this->tab_id, main);
}

QMap<QString,QVariant> Orgelwerk::getParams()
{
    QMap<QString,QVariant> result;

    result["key_shift"] = this->key_shift_master->value();
    result["volume"] = this->volume->getValue();
    result["keys"] = this->keys->getBitmaskOfKeys();

    QMap<QString,int> pitchwheel = this->pitch->getData();
    QList<QString> keys = pitchwheel.keys();
    for (const QString &key : keys)
    {
        result[key] = pitchwheel.value(key);
    }

    return result;
}

// ---------------------------------------------------------------------------
// changes made here

void Orgelwerk::keyShiftChanged(int value)
{
    if (this->loading || this->tab_id < 0)
        return;

    // set by the user: also the value a relative global reset returns to
    const int previous = this->engine->preset(this->tab_id).master_key_shift;
    this->engine->setMasterKeyShift(this->tab_id, value);
    
    // only the octave hotkeys (Ctrl) move the other sounding tabs along;
    // the buttons and the spin box of this view belong to the bound tab alone
    if (this->key_shift_from_hotkey)
        followKeyShift(this->engine->preset(this->tab_id).master_key_shift - previous);
}

void Orgelwerk::volumeSliderMoved(int value)
{
    if (this->loading || this->tab_id < 0)
        return;

    // the bound tab only; several tabs together: group DCA in MainTabs
    this->engine->setMasterVolume(this->tab_id, value);
}

void Orgelwerk::pitchWheelMoved(int pitch)
{
    if (this->loading)
        return;

    // MainTabs bends the current tab and all additionally checked ones
    emit signalMIDIEvent(pitch, MIDIMode::PitchBend);
}

void Orgelwerk::panicPressed()
{
    resetNoteDisplays();
    this->pc->allKeysUp();

    if (this->tab_id >= 0)
        this->engine->panic(this->tab_id);
}
void Orgelwerk::stopAllPressed()
{
    resetNoteDisplays();
    this->pc->allKeysUp();

    if (this->tab_id >= 0)
        this->engine->stopAll(this->tab_id);
}

void Orgelwerk::resendMIDIControls()
{
    if (this->tab_id < 0)
        return;

    storeToEngine();
    this->engine->resendControls(this->tab_id);
}

void Orgelwerk::channelsSummaryUpdate()
{
    if (this->tab_id < 0)
        return;

    QList<QMap<QString,QVariant>> data = PresetSerializer::toChannelMaps(this->engine->preset(this->tab_id), true);
    this->midi_channels_summary->showData(data);
}

// ---------------------------------------------------------------------------
// engine -> displays

void Orgelwerk::engineNoteOn(int tab, int note)
{
    if (tab == this->tab_id && this->notes)
        this->notes->keyPressed(note);
}
void Orgelwerk::engineNoteOff(int tab, int note)
{
    if (tab == this->tab_id && this->notes)
        this->notes->keyReleased(note);
}
void Orgelwerk::engineSourceKey(int tab, int source_key, bool pressed)
{
    if (tab != this->tab_id || !this->piano || !this->pc)
        return;

    const bool from_midi_input = source_key >= MidiEngine::MIDI_INPUT_HELD_KEY_BASE;
    const int note = MidiEngine::displayNoteForSourceKey(source_key);

    if (pressed)
    {
        if (this->displayed_source_notes.contains(source_key))
            return;

        this->displayed_source_notes.insert(source_key, note);
        const int count = this->displayed_note_counts.value(note);
        this->displayed_note_counts.insert(note, count + 1);
        if (count == 0)
            this->piano->keyPressed(note);
    }
    else
    {
        if (!this->displayed_source_notes.contains(source_key))
            return;

        const int old_note = this->displayed_source_notes.take(source_key);
        const int count = this->displayed_note_counts.value(old_note);
        if (count <= 1)
        {
            this->displayed_note_counts.remove(old_note);
            this->piano->keyReleased(old_note);
        }
        else
            this->displayed_note_counts.insert(old_note, count - 1);
    }

    // MIDI input affects the display only; it must not send another note back
    // through the PC keyboard's MIDIPress/MIDIRelease signals.
    if (from_midi_input)
        this->pc->setMidiNoteDown(note, pressed);
}
void Orgelwerk::enginePresetReplaced(int tab)
{
    if (tab == this->tab_id)
        reloadFromEngine();
}

void Orgelwerk::resetNoteDisplays()
{
    this->displayed_source_notes.clear();
    this->displayed_note_counts.clear();
    if (this->piano)
        this->piano->allKeysUp();
    if (this->pc)
        this->pc->clearMidiNoteHighlights();
    if (this->notes)
        this->notes->allKeysUp();
}

// ---------------------------------------------------------------------------

void Orgelwerk::keyDownRaw(int keycode)
{
    this->pc->keyDownRaw(keycode);
}
void Orgelwerk::keyUpRaw(int keycode)
{
    this->pc->keyUpRaw(keycode);
}

void Orgelwerk::keySustain(bool pressed)
{
    const QList<int> tabs = pedalTabs(pressed, this->sustain_tabs);
    for (const int tab : tabs)
        this->engine->sustain(tab, pressed);
}
void Orgelwerk::keySostenuto(bool pressed)
{
    const QList<int> tabs = pedalTabs(pressed, this->sostenuto_tabs);
    for (const int tab : tabs)
        this->engine->sostenuto(tab, pressed);
}
void Orgelwerk::keySoft(bool pressed)
{
    const QList<int> tabs = pedalTabs(pressed, this->soft_tabs);
    for (const int tab : tabs)
        this->engine->soft(tab, pressed);
}

void Orgelwerk::octaveDown()
{
    // hotkey (or the Ctrl key of the PC keyboard display): all sounding tabs
    this->key_shift_from_hotkey = true;
    this->key_shift_master->lowerShiftKeyPressed();
    this->key_shift_from_hotkey = false;
}
void Orgelwerk::octaveUp()
{
    this->key_shift_from_hotkey = true;
    this->key_shift_master->higherShiftKeyPressed();
    this->key_shift_from_hotkey = false;
}

void Orgelwerk::showHideGUIElements(GUIElements elements, bool show)
{
    QWidget *element = nullptr;

    if (elements == GUIElements::MIDIKeys)
        element = this->group_keys;
    else if (elements == GUIElements::PitchAndVolume)
        element = this->group_pitch;
    else if (elements == GUIElements::Notes)
        element = this->notes;
    else if (elements == GUIElements::KeyboardPC)
        element = this->pc;
    else if (elements == GUIElements::KeyboardPiano)
        element = this->piano;

    if (element)
        element->setVisible(show);
}

void Orgelwerk::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (this->check_resend_midi_auto->isChecked())
    {
        resendMIDIControls();
    }
}

bool Orgelwerk::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::KeyPress || ev->type() == QEvent::KeyRelease)
    {
        emit eventFiltered(obj, ev);

        return true;
    }

    return false;
}
