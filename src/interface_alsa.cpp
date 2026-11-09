#include "interface_alsa.h"

#include <cerrno>

/*
 * http://jack-keyboard.sourceforge.net/
 * https://stackoverflow.com/questions/21161118/send-midi-messages-from-c
 * https://tldp.org/HOWTO/MIDI-HOWTO-9.html
 * https://www.alsa-project.org/alsa-doc/alsa-lib/group___seq_middle.html
 * https://www.alsa-project.org/alsa-doc/alsa-lib/seq.html
 */

InterfaceAlsa::InterfaceAlsa(int id, InterfaceAudio *parent)
    : InterfaceAudio(id, parent)
{
    //this->label_string = label;
    
    int error = snd_seq_open(&this->seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
    if (error < 0)
    {
        qWarning() << "Cannot open the ALSA sequencer:" << snd_strerror(error);
        this->seq = nullptr;
    }
    else
    {
        snd_seq_set_client_name(this->seq, this->NAME.toLatin1());
    }
    
    /*
    int port;
    port = snd_seq_create_simple_port(this->seq, this->label_string.toLatin1(),
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION);
    Q_UNUSED(port);
    */
    
    snd_seq_ev_clear(&this->ev);
    snd_seq_ev_set_direct(&this->ev);
    
    /* either */
    //snd_seq_ev_set_dest(&this->ev, 64, 0); /* send to 64:0 */
    /* or */
    snd_seq_ev_set_subs(&this->ev);        /* send to subscribers of source port */
}

void InterfaceAlsa::createNewPort(QString label)
{
    if (!this->seq)
        return;
    
    int port = snd_seq_create_simple_port(this->seq, label.toLatin1(),
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION);
    if (port < 0)
        qWarning() << "Cannot create ALSA port" << label << ":" << snd_strerror(port);
}

InterfaceAlsa::~InterfaceAlsa()
{
    if (this->seq_in)
    {
        for (QSocketNotifier *notifier : this->input_notifiers)
            notifier->setEnabled(false);
        snd_seq_close(this->seq_in);
        this->seq_in = nullptr;
    }
    
    if (!this->seq)
        return;
    
    for (int i=0; i < 16; i++)
    {
        snd_seq_ev_set_controller(&this->ev, i, MIDI_CTL_ALL_NOTES_OFF, 127);
        sendEvent(true);
    }
    snd_seq_close(this->seq);
}

void InterfaceAlsa::keyPressEvent(int port, int channel, uint8_t midicode, uint8_t velocity)
{
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_noteon(&this->ev, channel, midicode, velocity);
    
    sendEvent(true);
}

void InterfaceAlsa::keyReleaseEvent(int port, int channel, uint8_t midicode, uint8_t velocity)
{
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_noteoff(&this->ev, channel, midicode, velocity);
    
    sendEvent(true);
}

void InterfaceAlsa::keyPanicEvent(int port, int channel)
{
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_ALL_NOTES_OFF, 127);
    sendEvent(true);
}

void InterfaceAlsa::keyStopAllEvent(int port, int channel)
{
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_ALL_SOUNDS_OFF, 127);
    sendEvent(true);
}

void InterfaceAlsa::keyPitchbendEvent(int port, int channel, int pitch)
{
    // In the midi-specs 8192 is defined as the neutral center.
    // However in the ALSA docs it says:
    // val: pitch bend; zero centered from -8192 to 8191 
    pitch = pitch - 8192;
    
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_pitchbend(&this->ev, channel, pitch);
    
    sendEvent(true);
}

void InterfaceAlsa::keySustainEvent(int port, int channel, bool pressed)
{
    int systain_value = 0;
    if (pressed)
        systain_value = 127;
    
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_SUSTAIN, systain_value);
    sendEvent(true);
}

void InterfaceAlsa::keySostenutoEvent(int port, int channel, bool pressed)
{
    int systain_value = 0;
    if (pressed)
        systain_value = 127;
    
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_SOSTENUTO, systain_value);
    sendEvent(true);
}

void InterfaceAlsa::keySoftEvent(int port, int channel, bool pressed)
{
    int value = 0;
    if (pressed)
        value = 127;
    
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_SOFT_PEDAL, value); // CC 67
    sendEvent(true);
}

void InterfaceAlsa::setProgramChangeEvent(int port, int channel, uint8_t program, int bank)
{
    int msb = (bank >> 7) & 0x7F;  // Bank / 128
    int lsb = bank & 0x7F;         // Bank % 128
    
    snd_seq_ev_set_source(&this->ev, port);
    
    // MSB send
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_MSB_BANK, msb);
    sendEvent(false);
    
    // LSB send
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_LSB_BANK, lsb);
    sendEvent(false);
    
    // Program Change
    snd_seq_ev_set_pgmchange(&this->ev, channel, program);
    sendEvent(true);
}

void InterfaceAlsa::setControlChangeEvent(int port, int channel, uint8_t cc, uint8_t value)
{
    cc    &= 0x7F;
    value &= 0x7F;
    
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_controller(&this->ev, channel, cc, value);
    sendEvent(true);
}


void InterfaceAlsa::setVolumeChangeEvent(int port, int channel, uint8_t volume)
{
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_MSB_MAIN_VOLUME, volume); // CC 7
    sendEvent(true);
}

void InterfaceAlsa::setPanChangeEvent(int port, int channel, uint8_t value)
{
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_MSB_PAN, value);
    sendEvent(true);
}

void InterfaceAlsa::setPortamentoChanged(int port, int channel, uint8_t value)
{
    snd_seq_ev_set_source(&this->ev, port);
    
    if (value > 0)
    {
        snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_PORTAMENTO, 127); // CC 5
        sendEvent(true);
    }
    else
    {
        snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_PORTAMENTO, 0); // CC 5
        sendEvent(true);
    }
    
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_MSB_PORTAMENTO_TIME, value); // CC 65
    sendEvent(true);
}

void InterfaceAlsa::setAttackChanged(int port, int channel, uint8_t value)
{
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_SC4_ATTACK_TIME, value);
    
    sendEvent(true);
}

void InterfaceAlsa::setReleaseChanged(int port, int channel, uint8_t value)
{
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_SC3_RELEASE_TIME, value);
    
    sendEvent(true);
}

void InterfaceAlsa::setTremoloChanged(int port, int channel, uint8_t value)
{
    snd_seq_ev_set_source(&this->ev, port);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_E2_TREMOLO_DEPTH, value);
    
    sendEvent(true);
}

void InterfaceAlsa::sendEvent(bool drain)
{
    if (!this->seq)
        return;
    
    snd_seq_ev_set_direct(&this->ev);
    snd_seq_event_output(this->seq, &this->ev);
    
    if (drain)
    {
        snd_seq_drain_output(this->seq);
    }
}

void InterfaceAlsa::createMidiInput(QString label)
{
    if (this->seq_in)
        return;
    
    int error = snd_seq_open(&this->seq_in, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK);
    if (error < 0)
    {
        qWarning() << "Cannot open the ALSA sequencer for MIDI input:" << snd_strerror(error);
        this->seq_in = nullptr;
        return;
    }
    snd_seq_set_client_name(this->seq_in, (this->NAME + "-in").toLatin1());
    
    int port = snd_seq_create_simple_port(this->seq_in, label.toLatin1(),
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
    if (port < 0)
    {
        qWarning() << "Cannot create ALSA MIDI input port" << label << ":" << snd_strerror(port);
        snd_seq_close(this->seq_in);
        this->seq_in = nullptr;
        return;
    }
    
    int count = snd_seq_poll_descriptors_count(this->seq_in, POLLIN);
    if (count <= 0)
        return;
    
    QList<struct pollfd> descriptors(count);
    snd_seq_poll_descriptors(this->seq_in, descriptors.data(), count, POLLIN);
    for (const struct pollfd &descriptor : descriptors)
    {
        QSocketNotifier *notifier = new QSocketNotifier(descriptor.fd, QSocketNotifier::Read, this);
        connect(notifier, &QSocketNotifier::activated, this, &InterfaceAlsa::readMidiInput);
        this->input_notifiers.append(notifier);
    }
}

void InterfaceAlsa::readMidiInput()
{
    if (!this->seq_in)
        return;
    
    // read everything that is pending; the handle is non-blocking
    while (snd_seq_event_input_pending(this->seq_in, 1) > 0)
    {
        snd_seq_event_t *event = nullptr;
        int result = snd_seq_event_input(this->seq_in, &event);
        if (result == -ENOSPC)
        {
            qWarning() << "ALSA MIDI input overrun, events lost";
            continue;
        }
        if (result < 0 || !event)
            break;
        
        switch (event->type)
        {
        case SND_SEQ_EVENT_NOTEON:
            dispatchMidiMessage(0x90, event->data.note.note, event->data.note.velocity);
            break;
        case SND_SEQ_EVENT_NOTEOFF:
            dispatchMidiMessage(0x80, event->data.note.note, event->data.note.velocity);
            break;
        case SND_SEQ_EVENT_CONTROLLER:
            dispatchMidiMessage(0xB0, event->data.control.param, event->data.control.value);
            break;
        case SND_SEQ_EVENT_PITCHBEND:
        {
            // ALSA: -8192..8191, MIDI: 0..16383
            int value = qBound(0, event->data.control.value + 8192, 16383);
            dispatchMidiMessage(0xE0, value & 0x7F, (value >> 7) & 0x7F);
            break;
        }
        default:
            break;
        }
    }
}
