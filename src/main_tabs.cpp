#include "main_tabs.h"

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
    
    // the one view of this keyboard
    this->view = new Orgelwerk(this->id, this->midi_engine, this);
    connect(this->view, &Orgelwerk::signalMIDIEvent, this, &MainTabs::routeMIDIEvent);
    connect(this->view, &Orgelwerk::signalEditChannels, this, &MainTabs::showChannelEditor);
    this->view->bindTab(0);
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
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
    
    emit currentChanged(index);
}

void MainTabs::listOfCheckTabsChanged(QList<int> list_of_tab_ids)
{
    this->list_of_checked_tabs = list_of_tab_ids;
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
            this->midi_engine->noteOn(tab, value);
        else if (mode == MIDIMode::PitchBend)
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

void MainTabs::globalKeyShiftChanged(int value, bool is_relative)
{
    this->midi_engine->globalKeyShift(value, is_relative);
    this->view->reloadFromEngine();
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
    
    QString udp_message = "keypress/"+QString::number(keycode);
    
    sendUDPMessage(udp_message);
    
    for (int i=0; i < this->list_function_keys_raw.length(); i++)
    {
        if (this->list_function_keys_raw.at(i) == keycode)
        {
            if (this->is_escape_pressed)
            {
                this->main_tabs_switcher->setLayerActive(i);
            }
            else
            {
                MIDISignal(MIDISignalTypes::Panick);
                
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
        
        MIDISignal(MIDISignalTypes::Panick);
        MIDISignal(MIDISignalTypes::StopAll);
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
        MIDISignal(MIDISignalTypes::ShowMenu);
    }
    else if (keycode == KeysRaw::Space)
    {
        this->is_space_pressed = true;
        
        MIDISignal(MIDISignalTypes::SustainPressed);
    }
    else if (keycode == KeysRaw::Alt)
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
    
    QString udp_message = "keyrelease/"+QString::number(keycode);
    
    sendUDPMessage(udp_message);
    
    if (keycode == KeysRaw::Escape)
    {
        this->is_escape_pressed = false;
    }
    else if (keycode == KeysRaw::Space)
    {
        this->is_space_pressed = false;
        
        MIDISignal(MIDISignalTypes::SustainReleased);
    }
    else if (keycode == KeysRaw::Alt)
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
    }
    else if (type == MIDISignalTypes::SostenutoReleased)
    {
        o->keySostenuto(false);
        o->keyUpRaw(KeysRaw::Alt);
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
    else if (type == MIDISignalTypes::VolumeLowerPressed)
    {
        o->volume->volumeKeyPressed(KeysRaw::Down);
    }
    else if (type == MIDISignalTypes::VolumeLowerPressedPermanent)
    {
        o->volume->volumeKeyPressed(KeysRaw::PageDown);
    }
    else if (type == MIDISignalTypes::VolumeHigherPressed)
    {
        o->volume->volumeKeyPressed(KeysRaw::Up);
    }
    else if (type == MIDISignalTypes::VolumeHigherPressedPermanent)
    {
        o->volume->volumeKeyPressed(KeysRaw::PageUp);
    }
    else if (type == MIDISignalTypes::VolumeReleased)
    {
        o->volume->volumeKeyReleased();
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
    }
    else if (type == MIDISignalTypes::OctaveUpPressed)
    {
        o->octaveUp();
    }
}


