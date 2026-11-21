#include "main_tabs.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSaveFile>

#include <QTimer>
#include <QScrollBar>

#include <linux/input.h>   // KEY_MAX

MainTabs::MainTabs(QList<QString> labels, int id, Config *config, MainTabsSwitcher *switcher, OutputSystem output, InputKeyboardSelect *input_keyboard_select, QLineEdit *line_udp_ip, QSpinBox *spin_port, QWidget *parent)
    : QWidget(parent)
{
    this->list_labels = labels;
    this->id = id;
    this->config = config;
    this->main_tabs_switcher = switcher;
    
    if (output == OutputSystem::Alsa)
    {
        this->interface_audio = new InterfaceAlsa(id);
    }
    else if (output == OutputSystem::Jack)
    {
        this->interface_audio = new InterfaceJack(id);
    }
#ifdef VKEYBD_HAS_LIBREMIDI
    else if (output == OutputSystem::Libremidi)
    {
        this->interface_audio = new InterfaceLibremidi(id, this);
    }
#endif
    else if (output == OutputSystem::Network)
    {
        this->send_udp = true;
        
        // no local sound output, but the engine still needs a (dummy) interface
        this->interface_audio = new InterfaceAudio(id, this);
    }
    else
    {
        this->interface_audio = new InterfaceAudio(id);
    }
    
    this->midi_engine = new MidiEngine(this->interface_audio, this);
    
    connect(input_keyboard_select, &InputKeyboardSelect::keyRawPressedSignal, this, &MainTabs::rawKeyPressed);
    connect(input_keyboard_select, &InputKeyboardSelect::keyRawReleasedSignal, this, &MainTabs::rawKeyReleased);
    
    this->line_udp_ip = line_udp_ip;
    this->spin_port = spin_port;
    
    this->socket = new QUdpSocket(this);
    
    initializeTabs();
    
    // MIDI inputs of this keyboard (the ports are opened with the keyboard state below)
    connect(this->interface_audio, &InterfaceAudio::signalMidiNoteOn, this, &MainTabs::midiInputNoteOn);
    connect(this->interface_audio, &InterfaceAudio::signalMidiNoteOff, this, &MainTabs::midiInputNoteOff);
    connect(this->interface_audio, &InterfaceAudio::signalMidiControlChange, this, &MainTabs::midiInputControlChange);
    connect(this->interface_audio, &InterfaceAudio::signalMidiPitchBend, this, &MainTabs::midiInputPitchBend);
    
    // the one view of this keyboard
    this->view = new Orgelwerk(this->id, this->midi_engine, this);
    this->view->setShowSoundingNotes(this->config->getPCKeyboardSoundingNotes());
    // keymap and keyboard form of this section
    loadKeyboardState();
    applyKeyboardState();
    openMidiInputPorts();
    // loading tabs may change which ones are played "always"
    connect(this->midi_engine, &MidiEngine::signalPresetReplaced, this, &MainTabs::updateFixedInputMarks);
    updateFixedInputMarks();
    connect(this->view->pcKeyboard(), &KeyboardPC::keyboardFormDetected, this, [this](KeyboardForm form){
        this->detected_form = KeyboardLayout::formName(form);
        saveKeyboardState();
    });
    // octave shift, volume and pedals apply to the current and all checked tabs
    this->view->setSoundingTabs([this]() { return soundingTabs(); });
    connect(this->view, &Orgelwerk::signalMIDIEvent, this, &MainTabs::routeMIDIEvent);
    connect(this->view, &Orgelwerk::signalEditChannels, this, &MainTabs::showChannelEditor);
    this->view->bindTab(0);
    
    // group DCA (hidden while only one tab sounds)
    this->group_volume = new MIDIMasterVolume(this);
    this->group_dca = new QWidget;
    this->group_dca->setObjectName("group_dca");
    QHBoxLayout *layout_dca = new QHBoxLayout;
    layout_dca->setContentsMargins(0, 0, 0, 0);
    layout_dca->setSpacing(4);
    layout_dca->addWidget(this->group_volume->label_volume);
    layout_dca->addWidget(this->group_volume->slider_volume, 1);
    this->group_dca->setLayout(layout_dca);
    // its spring-back strength comes from the tab's tether slider (see MIDISignal)
    this->group_volume->label_tether->setParent(this->group_dca);
    this->group_volume->slider_tether->setParent(this->group_dca);
    this->group_volume->label_tether->hide();
    this->group_volume->slider_tether->hide();
    this->group_dca->setToolTip("Group DCA: volume of all sounding tabs (current + checked) together.\n"
                                "Keeps their volume ratios; 100 % = their own volumes.\n"
                                "Hotkeys: Up/Down (springs back), Page Up/Page Down (stays).");
    this->group_dca->hide();
    connect(this->group_volume, &MIDIMasterVolume::sliderMoved, this, &MainTabs::groupDCAMoved);
    updateGroupDCALabel();
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(this->group_dca);
    layout->addWidget(this->view);
    setLayout(layout);
    
    if (output != OutputSystem::Network)
    {
        connect(line_udp_ip, &QLineEdit::textChanged, this, &MainTabs::rebindSocketIP);
        connect(spin_port, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainTabs::rebindSocket);
        
        this->socket->bind(QHostAddress::LocalHost, spin_port->value());
        connect(this->socket, &QUdpSocket::readyRead, this, &MainTabs::receiveUDPMessage);
    }
    else
    {
        hide();
    }
}

void MainTabs::initializeTabs()
{
    this->list_function_keys_raw = {KeysRaw::F1, KeysRaw::F2, KeysRaw::F3, KeysRaw::F4, KeysRaw::F5, KeysRaw::F6, KeysRaw::F7, KeysRaw::F8, KeysRaw::F9, KeysRaw::F10, KeysRaw::F11, KeysRaw::F12};
    
    // one MIDI port and one preset per tab. The port number is the tab id,
    // so the ports have to be created in this order.
    for (int tab_id = 0; tab_id < this->list_labels.length(); tab_id++)
    {
        this->interface_audio->createNewPort(QString::number(this->id + 1) + "-" + this->list_labels.at(tab_id));
        this->midi_engine->setPreset(tab_id, TabPreset());
    }
}

int MainTabs::count() const
{
    return this->list_labels.length();
}

int MainTabs::currentIndex() const
{
    return this->current_index;
}

void MainTabs::setCurrentIndex(int index)
{
    if (index < 0 || index >= count() || index == this->current_index)
        return;
    
    this->current_index = index;
    this->view->bindTab(index);
    
    if (this->midi_engine->preset(index).resend_midi_auto)
    {
        this->midi_engine->resendControls(index);
    }
    
    syncChannelEditorToCurrentTab();
    updateGroupDCA();
    
    emit currentChanged(index);
}

void MainTabs::listOfCheckTabsChanged(QList<int> list_of_tab_ids)
{
    this->list_of_checked_tabs = list_of_tab_ids;
    updateGroupDCA();
    this->view->soundingTabsChanged();
}

void MainTabs::updateGroupDCA()
{
    const QList<int> tabs = soundingTabs();
    const bool group = tabs.size() > 1;
    
    // tabs that left the group keep their volume, and are re-anchored if they come back
    const QList<int> anchored = this->group_anchors.keys();
    for (const int tab : anchored)
    {
        if (!tabs.contains(tab))
            this->group_anchors.remove(tab);
    }
    
    if (!group)
    {
        // the group is gone: the volumes stay as they are, the next group starts at 100 %
        this->group_anchors.clear();
        this->group_dca_value = 100;
        this->group_volume->setValueSilently(100);
    }
    
    updateGroupDCALabel();
    this->group_dca->setVisible(group);
}

void MainTabs::updateGroupDCALabel()
{
    this->group_volume->label_volume->setText(QString("Group DCA (%1 tabs): %2%").arg(soundingTabs().size()).arg(this->group_dca_value));
}

void MainTabs::groupDCAMoved(int value)
{
    const QList<int> tabs = soundingTabs();
    const int previous = this->group_dca_value;
    this->group_dca_value = value;
    updateGroupDCALabel();
    
    if (tabs.size() < 2)
        return;
    
    for (const int tab : tabs)
    {
        const int actual = this->midi_engine->preset(tab).master_volume;
        
        // anchor = the tab's volume at 100 %. New in the group, or changed on
        // its own slider meanwhile: anchored again from where it is now.
        bool anchor_valid = this->group_anchors.contains(tab);
        if (anchor_valid)
        {
            const int expected = qBound(0, qRound(this->group_anchors.value(tab) * previous / 100.0), 120);
            anchor_valid = (expected == actual);
        }
        if (!anchor_valid)
            this->group_anchors.insert(tab, (previous > 0) ? actual * 100.0 / previous : actual);
        
        const int scaled = qBound(0, qRound(this->group_anchors.value(tab) * value / 100.0), 120);
        this->midi_engine->setMasterVolume(tab, scaled);
    }
    
    // the slider of the current tab shows its new volume
    this->view->refreshVolume();
}

QList<int> MainTabs::soundingTabs() const
{
    QList<int> result;
    result.append(this->current_index);
    
    for (int tab : this->list_of_checked_tabs)
    {
        if (tab >= 0 && tab < count() && !result.contains(tab))
            result.append(tab);
    }
    
    return result;
}

void MainTabs::routeMIDIEvent(int value, MIDIMode mode)
{
    if (mode == MIDIMode::Release)
    {
        // not tab-specific: releases whatever was sent for this key, on whatever tab
        this->midi_engine->noteOff(value);
        return;
    }
    
    QList<int> tabs = soundingTabs();
    for (int tab : tabs)
    {
        if (mode == MIDIMode::Press)
            this->midi_engine->noteOn(tab, value, this->pc_velocity);
        else if (mode == MIDIMode::PitchBend)
            this->midi_engine->pitchBend(tab, value);
    }
}

QList<int> MainTabs::tabsForInput(int input) const
{
    const QList<int> sounding = soundingTabs();
    QList<int> tabs;
    for (int tab = 0; tab < this->list_labels.length(); tab++)
    {
        const QMap<int, MidiInputMode> inputs = this->midi_engine->preset(tab).midi_inputs;
        if (!inputs.contains(input))
            continue;
        if (inputs.value(input) == MidiInputMode::Always || sounding.contains(tab))
            tabs.append(tab);
    }
    return tabs;
}

void MainTabs::midiInputNoteOn(int input, int note, int velocity)
{
    if (this->keymap_editor && this->keymap_editor->isLearning())
    {
        this->keymap_editor->learnMidiNote(note);
        return;
    }
    const QList<int> tabs = tabsForInput(input);
    for (int tab : tabs)
    {
        this->midi_engine->midiNoteOn(tab, input, note, velocity);
    }
}
void MainTabs::midiInputNoteOff(int input, int note)
{
    // releases exactly what was sent for this note of this input, on whatever tabs
    this->midi_engine->midiNoteOff(input, note);
}
void MainTabs::midiInputControlChange(int input, int cc, int value)
{
    if (this->midi_routing_dialog && this->midi_routing_dialog->isLearning())
    {
        this->midi_routing_dialog->learnControl(input, cc);
        return;
    }
    if (controlFunction(input, cc, value))
        return;
    
    // the pedals go to the tabs of the input, like the ones on the PC keyboard
    const int SUSTAIN = 64;
    const int SOSTENUTO = 66;
    const int SOFT = 67;
    
    const QList<int> tabs = tabsForInput(input);
    for (int tab : tabs)
    {
        if (cc == SUSTAIN)
            this->midi_engine->sustain(tab, value >= 64);
        else if (cc == SOSTENUTO)
            this->midi_engine->sostenuto(tab, value >= 64);
        else if (cc == SOFT)
            this->midi_engine->soft(tab, value >= 64);
        else
            this->midi_engine->forwardControlChange(tab, cc, value);
    }
}
void MainTabs::midiInputPitchBend(int input, int value)
{
    const QList<int> tabs = tabsForInput(input);
    for (int tab : tabs)
    {
        this->midi_engine->pitchBend(tab, value);
    }
}

void MainTabs::saveParams(QSettings *settings)
{
    // the pitch wheel position of the current tab is not written back continuously
    this->view->storeToEngine();
    
    for (int tab_id = 0; tab_id < this->list_labels.length(); tab_id++)
    {
        PresetSerializer::save(settings, this->id, this->list_labels.at(tab_id), this->midi_engine->preset(tab_id));
    }
    
    QMap<QString,QVariant> params;
    params["network_ip"] = this->line_udp_ip->text();
    params["network_port"] = this->spin_port->value();
    
    this->config->saveParams(settings, this->id, "general", "", params);
}
void MainTabs::restoreParams(QString tab, QMap<QString,QVariant> data)
{
    int tab_id = this->list_labels.indexOf(tab);
    if (tab_id < 0)
        return;
    
    // the view reloads itself if it shows this tab (signalPresetReplaced)
    this->midi_engine->setPreset(tab_id, PresetSerializer::fromRestoreData(data));
    
    if (this->channel_editor && this->channel_editor->boundTab() == tab_id)
        this->channel_editor->bindTab(tab_id);
}

void MainTabs::showHideGUIElements(GUIElements elements, bool show)
{
    // no resize() here: forcing the size of a widget inside a layout made it collapse
    this->view->showHideGUIElements(elements, show);
}

void MainTabs::sendUDPMessage(QString message)
{
    QByteArray udp_message;
    udp_message = message.toUtf8();
    
    if (this->send_udp)
    {
        this->socket->writeDatagram(udp_message, QHostAddress(this->line_udp_ip->text()), this->spin_port->value());
    }
}

void MainTabs::receiveUDPMessage()
{
    // the sender (an instance started with "-o network") sends
    // "keypress/<linux key code>" and "keyrelease/<linux key code>"
    while (this->socket->hasPendingDatagrams())
    {
        QNetworkDatagram datagram = this->socket->receiveDatagram(MAX_UDP_MESSAGE_SIZE);
        QList<QByteArray> parts = datagram.data().trimmed().split('/');
        
        // anything else that arrives on the port is ignored
        if (parts.length() != 2)
            continue;
        
        bool ok = false;
        int keycode = parts.at(1).toInt(&ok);
        if (!ok || keycode < 0 || keycode > KEY_MAX)
            continue;
        
        if (parts.at(0) == "keypress")
            rawKeyPressed(keycode);
        else if (parts.at(0) == "keyrelease")
            rawKeyReleased(keycode);
    }
}

void MainTabs::rebindSocketIP(QString ip)
{
    socket->close();
    socket->bind(QHostAddress(ip), this->spin_port->value());
}
void MainTabs::rebindSocket(int value)
{
    socket->close();
    socket->bind(QHostAddress(this->line_udp_ip->text()), value);
}

void MainTabs::deviceNotAvailable(QString message)
{
    qDebug() << "main_tabs: deviceNotAvailable";
}

void MainTabs::setGlobalKeyShift(int value)
{
    // the tabs keep their own key shift: nothing to reload in the view
    this->midi_engine->setGlobalKeyShift(value);
}
QString MainTabs::currentLabel() const
{
    if (this->current_index < 0 || this->current_index >= this->list_labels.length())
        return QString();
    return this->list_labels.at(this->current_index);
}

int MainTabs::globalKeyShiftGet()
{
    return this->midi_engine->preset(this->current_index).master_key_shift;
}

void MainTabs::globalResendMIDISettings()
{
    this->view->storeToEngine();
    
    // with ALSA we have to wait between the tabs or the MIDI signals get dropped.
    // staggered by timers instead of usleep(), so the GUI does not freeze
    const int STAGGER_MS = 10;
    MidiEngine *engine = this->midi_engine;
    for (int tab_id = 0; tab_id < count(); tab_id++)
    {
        QTimer::singleShot(tab_id * STAGGER_MS, engine, [engine, tab_id]{ engine->resendControls(tab_id); });
    }
}

void MainTabs::showChannelEditor(int tab_id)
{
    if (!this->channel_editor)
    {
        this->channel_editor = new MIDIChannelSelector(this->midi_engine);
        connect(this->channel_editor, &MIDIChannelSelector::closed, this, &MainTabs::channelEditorClosed);
        
        this->scroll_channel_editor = new QScrollArea(this);
        this->scroll_channel_editor->setWindowFlags(Qt::Window);
        this->scroll_channel_editor->setWidget(this->channel_editor);
        this->scroll_channel_editor->setWidgetResizable(false);
    }
    
    bindChannelEditor(tab_id);
    
    // size the window to the editor (plus scroll bars, if any)
    QMargins margins = this->scroll_channel_editor->contentsMargins();
    int width = this->channel_editor->sizeHint().width() + margins.left() + margins.right();
    int height = this->channel_editor->sizeHint().height() + margins.top() + margins.bottom();
    
    QScrollBar *scrollbar_horizontal = this->scroll_channel_editor->horizontalScrollBar();
    if (!scrollbar_horizontal->isHidden())
        height += scrollbar_horizontal->sizeHint().height();
    
    QScrollBar *scrollbar_vertical = this->scroll_channel_editor->verticalScrollBar();
    if (!scrollbar_vertical->isHidden())
        width += scrollbar_vertical->sizeHint().width();
    
    this->scroll_channel_editor->resize(width, height);
    this->scroll_channel_editor->show();
    this->scroll_channel_editor->raise();
    this->scroll_channel_editor->activateWindow();
}

void MainTabs::syncChannelEditorToCurrentTab()
{
    if (!this->channel_editor || !this->scroll_channel_editor->isVisible())
        return;
    
    if (this->channel_editor->boundTab() == this->current_index)
        return;
    
    bindChannelEditor(this->current_index);
    
    // the view shows the current tab: its summary is refreshed on binding anyway
}

void MainTabs::bindChannelEditor(int tab_id)
{
    this->channel_editor->bindTab(tab_id);
    
    QString label = (tab_id >= 0 && tab_id < this->list_labels.length()) ? this->list_labels.at(tab_id) : QString::number(tab_id);
    this->scroll_channel_editor->setWindowTitle("Keyboard " + QString::number(this->id + 1) + " - " + label);
}

void MainTabs::channelEditorClosed()
{
    if (this->scroll_channel_editor)
        this->scroll_channel_editor->hide();
    
    this->view->channelsSummaryUpdate();
}

void MainTabs::rawKeyPressed(int keycode)
{
    // the keymap editor is being used: the key selects a key there, nothing plays
    if (this->keymap_editor && this->keymap_editor->isActiveWindow())
    {
        this->keys_to_editor.insert(keycode);
        this->keymap_editor->physicalKeyPressed(keycode);
        return;
    }
    
    QString udp_message = "keypress/"+QString::number(keycode);
    
    sendUDPMessage(udp_message);
    
    for (int i=0; i < this->list_function_keys_raw.length(); i++)
    {
        if (this->list_function_keys_raw.at(i) == keycode)
        {
            if (this->is_escape_pressed)
            {
                // Esc + F-key: row switch (also while Menu is held for multiselect)
                this->main_tabs_switcher->setLayerActive(i);
                this->escape_used_for_row = true;
            }
            else if (this->is_menu_pressed)
            {
                // like a right click: play this tab additionally, or not anymore
                int layer = this->main_tabs_switcher->getLayerActive();
                this->main_tabs_switcher->toggleTabChecked(i + layer*12);
            }
            else
            {
                // no panic: held notes keep sounding on the previous tab until
                // their keys are released (the engine remembers where each key
                // played), only new presses play the new tab -- like the key shift
                int layer = this->main_tabs_switcher->getLayerActive();
                this->setCurrentIndex(i + layer*12);
            }
            
            /*
            if (this->is_space_pressed)
            {
                MIDISignal(MIDISignalTypes::SustainReleased);
                this->setCurrentIndex(i+12);
            }
            else if (this->is_escape_pressed)
            {
                this->setCurrentIndex(i+24);
            }
            else
                this->setCurrentIndex(i);
            */
        }
    }
    
    if (keycode == KeysRaw::Escape)
    {
        this->is_escape_pressed = true;
        this->escape_used_for_row = false;
        this->escape_with_menu = this->is_menu_pressed;
        
        // with Menu held: no panic, and the reset to the current tab waits for
        // the release of Esc, as Esc + F-key may still switch the row
        if (!this->escape_with_menu)
        {
            MIDISignal(MIDISignalTypes::Panick);
            MIDISignal(MIDISignalTypes::StopAll);
        }
    }
    else if (keycode == KeysRaw::Delete)
    {
        MIDISignal(MIDISignalTypes::StopAll);
    }
    else if (keycode == KeysRaw::Insert)
    {
        MIDISignal(MIDISignalTypes::ResendMIDISettings);
    }
    else if (keycode == KeysRaw::Menu)
    {
        // modifier: Menu + F-key adds/removes a tab, Menu + Esc resets to the current tab
        this->is_menu_pressed = true;
        this->view->keyDownRaw(keycode);
    }
    else if (keycode == KeysRaw::Space)
    {
        this->is_space_pressed = true;
        
        MIDISignal(MIDISignalTypes::SustainPressed);
    }
    else if (keycode == KeysRaw::Alt || keycode == KeysRaw::AltGr)
    {
        MIDISignal(MIDISignalTypes::SostenutoPressed);
    }
    else if (keycode == KeysRaw::Super_L || keycode == KeysRaw::Super_R)
    {
        MIDISignal(MIDISignalTypes::SoftPressed);
    }
    else if (keycode == KeysRaw::Left)
    {
        MIDISignal(MIDISignalTypes::PitchLowerPressed);
    }
    else if (keycode == KeysRaw::Right)
    {
        MIDISignal(MIDISignalTypes::PitchHigherPressed);
    }
    else if (keycode == KeysRaw::Down)
    {
        MIDISignal(MIDISignalTypes::VolumeLowerPressed);
    }
    else if (keycode == KeysRaw::Up)
    {
        MIDISignal(MIDISignalTypes::VolumeHigherPressed);
    }
    else if (keycode == KeysRaw::PageDown)
    {
        MIDISignal(MIDISignalTypes::VolumeLowerPressedPermanent);
    }
    else if (keycode == KeysRaw::PageUp)
    {
        MIDISignal(MIDISignalTypes::VolumeHigherPressedPermanent);
    }
    else if (keycode == KeysRaw::End)
    {
        MIDISignal(MIDISignalTypes::PitchLowerPressedPermanent);
    }
    else if (keycode == KeysRaw::Home)
    {
        MIDISignal(MIDISignalTypes::PitchHigherPressedPermanent);
    }
    else if (keycode == KeysRaw::Control_L)
    {
        MIDISignal(MIDISignalTypes::OctaveDownPressed);
        
        // to animate the button press on the virtual keyboard
        this->view->keyDownRaw(keycode);
    }
    else if (keycode == KeysRaw::Control_R)
    {
        MIDISignal(MIDISignalTypes::OctaveUpPressed);
        
        // to animate the button press on the virtual keyboard
        this->view->keyDownRaw(keycode);
    }
    else if (keycode == KeysRaw::Print)
    {
        emit signalKeyShiftChanged(-1, true);
        
        // Notification in MainWindow::globalKeyShiftValueChanged
    }
    else if (keycode == KeysRaw::Scroll)
    {
        emit signalKeyShiftChanged(0, true);
        
        // Notification in MainWindow::globalKeyShiftValueChanged
    }
    else if (keycode == KeysRaw::Pause)
    {
        emit signalKeyShiftChanged(1, true);
        
        // Notification in MainWindow::globalKeyShiftValueChanged
    }
    else
    {
        this->view->keyDownRaw(keycode);
    }
}
void MainTabs::rawKeyReleased(int keycode)
{
    if (this->keys_to_editor.remove(keycode))
        return;
    
    QString udp_message = "keyrelease/"+QString::number(keycode);
    
    sendUDPMessage(udp_message);
    
    if (keycode == KeysRaw::Escape)
    {
        this->is_escape_pressed = false;
        
        // Menu + Esc tapped (no F-key in between): only the current tab sounds again
        // (no panic: it keeps playing)
        if (this->escape_with_menu && !this->escape_used_for_row)
            this->main_tabs_switcher->pressButton(this->currentIndex());
        this->escape_with_menu = false;
    }
    else if (keycode == KeysRaw::Menu)
    {
        this->is_menu_pressed = false;
    }
    else if (keycode == KeysRaw::Space)
    {
        this->is_space_pressed = false;
        
        MIDISignal(MIDISignalTypes::SustainReleased);
    }
    else if (keycode == KeysRaw::Alt || keycode == KeysRaw::AltGr)
    {
        MIDISignal(MIDISignalTypes::SostenutoReleased);
    }
    else if (keycode == KeysRaw::Super_L || keycode == KeysRaw::Super_R)
    {
        MIDISignal(MIDISignalTypes::SoftReleased);
    }
    else if (keycode == KeysRaw::Left || keycode == KeysRaw::Right || keycode == KeysRaw::End || keycode == KeysRaw::Home)
    {
        MIDISignal(MIDISignalTypes::PitchReleased);
    }
    else if (keycode == KeysRaw::Up || keycode == KeysRaw::Down || keycode == KeysRaw::PageUp || keycode == KeysRaw::PageDown)
    {
        MIDISignal(MIDISignalTypes::VolumeReleased);
    }
    
    // the engine releases exactly what was sent for this key, even if the tab changed meanwhile
    this->view->keyUpRaw(keycode);
}

void MainTabs::MIDISignal(MIDISignalTypes type)
{
    Orgelwerk *o = this->view;
    
    if (type == MIDISignalTypes::ToggleKeyboardLock)
    {
        
    }
    else if (type == MIDISignalTypes::Panick)
    {
        // all tabs, immediately (animateClick() would emit clicked() only ~100 ms later,
        // which would cut the first notes played on the newly selected tab)
        o->resetNoteDisplays();
        for (int tab_id = 0; tab_id < count(); tab_id++)
        {
            this->midi_engine->panic(tab_id);
        }
    }
    else if (type == MIDISignalTypes::StopAll)
    {
        o->resetNoteDisplays();
        for (int tab_id = 0; tab_id < count(); tab_id++)
        {
            this->midi_engine->stopAll(tab_id);
        }
    }
    else if (type == MIDISignalTypes::ShowMenu)
    {
        //o->button_channels_dialog->animateClick();
    }
    else if (type == MIDISignalTypes::ResendMIDISettings)
    {
        o->button_resend_midi->animateClick();
    }
    else if (type == MIDISignalTypes::SustainPressed)
    {
        o->keySustain(true);
        o->keyDownRaw(KeysRaw::Space);
    }
    else if (type == MIDISignalTypes::SustainReleased)
    {
        o->keySustain(false);
        o->keyUpRaw(KeysRaw::Space);
    }
    else if (type == MIDISignalTypes::SostenutoPressed)
    {
        o->keySostenuto(true);
        o->keyDownRaw(KeysRaw::Alt);
        o->keyDownRaw(KeysRaw::AltGr);
    }
    else if (type == MIDISignalTypes::SostenutoReleased)
    {
        o->keySostenuto(false);
        o->keyUpRaw(KeysRaw::Alt);
        o->keyUpRaw(KeysRaw::AltGr);
    }
    else if (type == MIDISignalTypes::SoftPressed)
    {
        o->keySoft(true);
        o->keyDownRaw(KeysRaw::Super_L);
        o->keyDownRaw(KeysRaw::Super_R);
    }
    else if (type == MIDISignalTypes::SoftReleased)
    {
        o->keySoft(false);
        o->keyUpRaw(KeysRaw::Super_L);
        o->keyUpRaw(KeysRaw::Super_R);
    }
    else if (type == MIDISignalTypes::VolumeLowerPressed ||
             type == MIDISignalTypes::VolumeLowerPressedPermanent ||
             type == MIDISignalTypes::VolumeHigherPressed ||
             type == MIDISignalTypes::VolumeHigherPressedPermanent)
    {
        int key = KeysRaw::Down;
        if (type == MIDISignalTypes::VolumeLowerPressedPermanent)
            key = KeysRaw::PageDown;
        else if (type == MIDISignalTypes::VolumeHigherPressed)
            key = KeysRaw::Up;
        else if (type == MIDISignalTypes::VolumeHigherPressedPermanent)
            key = KeysRaw::PageUp;
        
        // several tabs sound: the hotkeys drive the group DCA, else the tab's own volume
        if (soundingTabs().size() > 1)
        {
            this->group_volume->slider_tether->setValue(o->volume->slider_tether->value());
            this->group_volume->volumeKeyPressed(key);
        }
        else
        {
            o->volume->volumeKeyPressed(key);
        }
    }
    else if (type == MIDISignalTypes::VolumeReleased)
    {
        o->volume->volumeKeyReleased();
        this->group_volume->volumeKeyReleased();
    }
    else if (type == MIDISignalTypes::PitchLowerPressed)
    {
        o->pitch->pitchKeyPressed(KeysRaw::Left);
    }
    else if (type == MIDISignalTypes::PitchLowerPressedPermanent)
    {
        o->pitch->pitchKeyPressed(KeysRaw::End);
    }
    else if (type == MIDISignalTypes::PitchHigherPressed)
    {
        o->pitch->pitchKeyPressed(KeysRaw::Right);
    }
    else if (type == MIDISignalTypes::PitchHigherPressedPermanent)
    {
        o->pitch->pitchKeyPressed(KeysRaw::Home);
    }
    else if (type == MIDISignalTypes::PitchReleased)
    {
        o->pitch->pitchKeyReleased();
    }
    else if (type == MIDISignalTypes::OctaveDownPressed)
    {
        o->octaveDown();
        emit signalStatusChanged();
    }
    else if (type == MIDISignalTypes::OctaveUpPressed)
    {
        o->octaveUp();
        emit signalStatusChanged();
    }
}



// ---------------------------------------------------------------------------
// keymap and keyboard form

QString MainTabs::keyboardStatePath() const
{
    return this->config->dataDir() + QString("/keyboard-%1.json").arg(this->id + 1);
}

void MainTabs::loadKeyboardState()
{
    QFile file(keyboardStatePath());
    if (!file.open(QIODevice::ReadOnly))
        return;   // first start: default keymap, automatic form
    
    const QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
    KeyboardForm form = KeyboardForm::ISO;
    const QString setting = json.value("form").toString();
    if (setting == "auto" || KeyboardLayout::formFromName(setting, &form))
        this->keyboard_form_setting = setting;
    if (KeyboardLayout::formFromName(json.value("detected").toString(), &form))
        this->detected_form = json.value("detected").toString();
    
    Keymap loaded;
    QString error;
    // MIDI input ports; the first one (id 0) always exists
    this->midi_input_ports.clear();
    const QJsonArray ports = json.value("midi_inputs").toArray();
    for (const QJsonValue &value : ports)
    {
        MidiInputPort port;
        port.id = value.toObject().value("id").toInt(-1);
        port.name = value.toObject().value("name").toString().trimmed();
        const QJsonObject controls = value.toObject().value("controls").toObject();
        for (QJsonObject::const_iterator it = controls.constBegin(); it != controls.constEnd(); ++it)
        {
            bool ok = false;
            const int cc = it.key().toInt(&ok);
            ControlFunction function = ControlFunction::Velocity;
            if (ok && cc >= 0 && cc <= 127 && MidiInputPort::functionFromName(it.value().toString(), &function))
                port.controls.insert(cc, function);
        }
        bool duplicate = false;
        for (const MidiInputPort &other : this->midi_input_ports)
            duplicate = duplicate || other.id == port.id || other.name == port.name;
        if (port.id >= 0 && !port.name.isEmpty() && !duplicate)
            this->midi_input_ports.append(port);
    }
    
    if (json.contains("keymap"))
    {
        if (Keymap::fromJson(json.value("keymap").toObject(), &loaded, &error))
            this->keymap = loaded;
        else
            qWarning() << "Ignoring the keymap in" << keyboardStatePath() << ":" << error;
    }
}

void MainTabs::saveKeyboardState()
{
    QJsonObject json;
    json.insert("form", this->keyboard_form_setting);
    if (!this->detected_form.isEmpty())
        json.insert("detected", this->detected_form);
    json.insert("keymap", this->keymap.toJson());
    QJsonArray ports;
    for (const MidiInputPort &port : this->midi_input_ports)
    {
        QJsonObject object;
        object.insert("id", port.id);
        object.insert("name", port.name);
        QJsonObject controls;
        for (QMap<int, ControlFunction>::const_iterator it = port.controls.cbegin(); it != port.controls.cend(); ++it)
            controls.insert(QString::number(it.key()), MidiInputPort::functionName(it.value()));
        if (!controls.isEmpty())
            object.insert("controls", controls);
        ports.append(object);
    }
    json.insert("midi_inputs", ports);
    
    QSaveFile file(keyboardStatePath());
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Cannot write" << keyboardStatePath() << ":" << file.errorString();
        return;
    }
    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    if (!file.commit())
        qWarning() << "Cannot write" << keyboardStatePath() << ":" << file.errorString();
}

KeyboardForm MainTabs::autoKeyboardForm() const
{
    KeyboardForm form = KeyboardForm::ISO;
    if (KeyboardLayout::formFromName(this->detected_form, &form))
        return form;
    return KeyboardLayout::guessSystemForm();
}

void MainTabs::applyKeyboardState()
{
    KeyboardPC *pc = this->view->pcKeyboard();
    KeyboardForm form = autoKeyboardForm();
    const bool automatic = !KeyboardLayout::formFromName(this->keyboard_form_setting, &form);
    pc->setAutoForm(automatic);
    pc->setKeyboardForm(form);
    pc->setKeymap(this->keymap);
}

void MainTabs::keymapApplied(const Keymap &keymap, const QString &form_setting)
{
    this->keymap = keymap;
    this->keyboard_form_setting = form_setting;
    saveKeyboardState();
    applyKeyboardState();
}

void MainTabs::openKeymapEditor()
{
    if (this->keymap_editor)
    {
        this->keymap_editor->show();
        this->keymap_editor->raise();
        this->keymap_editor->activateWindow();
        return;
    }
    
    this->keymap_editor = new KeymapEditor(this->id, this->keymap, this->keyboard_form_setting,
                                           autoKeyboardForm(), this->config->dataDir() + "/keymaps", window());
    this->keymap_editor->setShowSoundingNotesChecked(this->view->pcKeyboard()->showSoundingNotes());
    connect(this->keymap_editor, &KeymapEditor::applied, this, &MainTabs::keymapApplied);
    connect(this->keymap_editor, &KeymapEditor::showSoundingNotesToggled, this, &MainTabs::signalShowSoundingNotes);
    this->keymap_editor->show();
}

void MainTabs::setShowSoundingNotes(bool show)
{
    this->view->setShowSoundingNotes(show);
    if (this->keymap_editor)
        this->keymap_editor->setShowSoundingNotesChecked(show);
}

// ---------------------------------------------------------------------------
// MIDI inputs

QString MainTabs::defaultMidiInputName() const
{
    // the name of the one input before there were several (keeps patchbay connections)
    return QString::number(this->id + 1) + "-MIDI-in";
}

void MainTabs::openMidiInputPorts()
{
    bool has_default = false;
    for (const MidiInputPort &port : this->midi_input_ports)
        has_default = has_default || port.id == TabPreset::DEFAULT_MIDI_INPUT;
    if (!has_default)
    {
        MidiInputPort port;
        port.id = TabPreset::DEFAULT_MIDI_INPUT;
        port.name = defaultMidiInputName();
        this->midi_input_ports.prepend(port);
    }
    
    for (const MidiInputPort &port : this->midi_input_ports)
        this->interface_audio->openMidiInput(port.id, port.name);
}

QString MainTabs::tabTitle(int tab) const
{
    // the label and the instrument of the first activated channel
    const TabPreset preset = this->midi_engine->preset(tab);
    for (const ChannelSettings &channel : preset.channels)
    {
        if (channel.activated)
            return this->list_labels.at(tab) + "  " + channel.instrument_name;
    }
    return this->list_labels.at(tab);
}

void MainTabs::openMidiRouting()
{
    if (this->midi_routing_dialog)
    {
        this->midi_routing_dialog->show();
        this->midi_routing_dialog->raise();
        this->midi_routing_dialog->activateWindow();
        return;
    }
    
    QStringList titles;
    QList<QMap<int, MidiInputMode>> routes;
    for (int tab = 0; tab < this->list_labels.length(); tab++)
    {
        titles.append(tabTitle(tab));
        routes.append(this->midi_engine->preset(tab).midi_inputs);
    }
    
    this->midi_routing_dialog = new MidiRoutingDialog(this->id, this->midi_input_ports, titles, routes, window());
    connect(this->midi_routing_dialog, &MidiRoutingDialog::applied, this, &MainTabs::midiRoutingApplied);
    this->midi_routing_dialog->show();
}

void MainTabs::midiRoutingApplied(const QList<MidiInputPort> &ports, const QList<QMap<int, MidiInputMode>> &routes)
{
    // ports: close the removed ones (after releasing their notes), rename, open the new ones
    for (const MidiInputPort &old_port : this->midi_input_ports)
    {
        bool kept = false;
        for (const MidiInputPort &port : ports)
            kept = kept || port.id == old_port.id;
        if (kept)
            continue;
        for (int note = 0; note < 128; note++)
            this->midi_engine->midiNoteOff(old_port.id, note);
        this->interface_audio->closeMidiInput(old_port.id);
    }
    for (const MidiInputPort &port : ports)
    {
        bool existed = false;
        for (const MidiInputPort &old_port : this->midi_input_ports)
        {
            if (old_port.id != port.id)
                continue;
            existed = true;
            if (old_port.name != port.name)
                this->interface_audio->renameMidiInput(port.id, port.name);
        }
        if (!existed)
            this->interface_audio->openMidiInput(port.id, port.name);
    }
    this->midi_input_ports = ports;
    saveKeyboardState();
    
    // a pedal function that is gone: back to normal
    bool has_velocity = false;
    bool has_swell = false;
    for (const MidiInputPort &port : this->midi_input_ports)
    {
        for (const ControlFunction function : port.controls)
        {
            has_velocity = has_velocity || function == ControlFunction::Velocity;
            has_swell = has_swell || function == ControlFunction::Swell;
        }
    }
    if (!has_velocity)
        this->pc_velocity = -1;
    if (!has_swell)
    {
        this->pending_swell = -1;
        this->midi_engine->setSwell(127);
    }
    
    // routing: part of the tab presets (saved with the session)
    for (int tab = 0; tab < routes.size() && tab < this->list_labels.length(); tab++)
    {
        if (this->midi_engine->preset(tab).midi_inputs != routes.at(tab))
            this->midi_engine->setMidiInputs(tab, routes.at(tab));
    }
    updateFixedInputMarks();
}

bool MainTabs::controlFunction(int input, int cc, int value)
{
    for (const MidiInputPort &port : this->midi_input_ports)
    {
        if (port.id != input || !port.controls.contains(cc))
            continue;
        
        switch (port.controls.value(cc))
        {
            case ControlFunction::Velocity:
                // the next PC notes; a pedal fully up still plays (velocity 1)
                this->pc_velocity = qBound(1, value, 127);
                break;
            case ControlFunction::Swell:
                if (this->pending_swell < 0)
                    QTimer::singleShot(SWELL_INTERVAL_MS, this, &MainTabs::applySwell);
                this->pending_swell = value;
                break;
        }
        return true;
    }
    return false;
}

void MainTabs::applySwell()
{
    if (this->pending_swell < 0)
        return;
    this->midi_engine->setSwell(this->pending_swell);
    this->pending_swell = -1;
}

void MainTabs::updateFixedInputMarks()
{
    QSet<int> tabs;
    for (int tab = 0; tab < this->list_labels.length(); tab++)
    {
        const QMap<int, MidiInputMode> inputs = this->midi_engine->preset(tab).midi_inputs;
        for (const MidiInputMode mode : inputs)
        {
            if (mode == MidiInputMode::Always)
                tabs.insert(tab);
        }
    }
    this->main_tabs_switcher->setFixedInputTabs(tabs);
}
