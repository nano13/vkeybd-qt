#include "interface_alsa.h"

/*
 * http://jack-keyboard.sourceforge.net/
 * https://stackoverflow.com/questions/21161118/send-midi-messages-from-c
 * https://tldp.org/HOWTO/MIDI-HOWTO-9.html
 * https://www.alsa-project.org/alsa-doc/alsa-lib/group___seq_middle.html
 * https://www.alsa-project.org/alsa-doc/alsa-lib/seq.html
 */

InterfaceAlsa::InterfaceAlsa(QString label, InterfaceAudio *parent) : InterfaceAudio(parent)
{
    this->label = label;
    
    qDebug() << "init interface alsa: "+this->label;
    
    snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
    snd_seq_set_client_name(seq, this->NAME.toLatin1());
    
    int port;
    port = snd_seq_create_simple_port(seq, this->label.toLatin1(),
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION);
    
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_direct(&ev);
}

InterfaceAlsa::~InterfaceAlsa()
{
    for (int i=0; i < 128; i++)
    {
        snd_seq_ev_set_controller(&ev, i, MIDI_CTL_ALL_NOTES_OFF, 127);
        snd_seq_event_output(seq, &ev);
        snd_seq_drain_output(seq);
    }
    snd_seq_close(seq);
}

void InterfaceAlsa::keyPressEvent(int channel, int midicode)
{
    /* either */
    //snd_seq_ev_set_dest(&ev, 64, 0); /* send to 64:0 */
    /* or */
    snd_seq_ev_set_subs(&ev);        /* send to subscribers of source port */
    
    //snd_seq_ev_set_controller(&ev, channel, MIDI_CTL_LSB_MAIN_VOLUME, 50);
    
    snd_seq_ev_set_noteon(&ev, channel, midicode, 127);
    //snd_seq_event_output(seq, &ev);
    //snd_seq_drain_output(seq);
    
    sendEvent(true);
    
    /*
    setProgramChangeEvent(0, 125, 0);
    setProgramChangeEvent(1, 125, 8);
    setProgramChangeEvent(2, 125, 16);
    setProgramChangeEvent(3, 125, 3);
    setProgramChangeEvent(4, 125, 4);
    setProgramChangeEvent(5, 125, 5);
    */
}

void InterfaceAlsa::keyReleaseEvent(int channel, int midicode)
{
    snd_seq_ev_set_noteoff(&ev, channel, midicode, 127);
    
    sendEvent(true);
}

void InterfaceAlsa::keyPanicEvent(int channel)
{
    snd_seq_ev_set_controller(&ev, channel, MIDI_CTL_ALL_NOTES_OFF, 127);
    
    sendEvent(true);
}

void InterfaceAlsa::keyPitchbendEvent(int channel, int pitch)
{
    snd_seq_ev_set_pitchbend(&ev, channel, pitch);
    
    sendEvent(true);
}

void InterfaceAlsa::setProgramChangeEvent(int channel, int program, int bank)
{
    qDebug() << "alsa program change event: "+QString::number(channel);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_MSB_BANK, 121);
    sendEvent(false);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_LSB_BANK, bank);
    sendEvent(false);
    snd_seq_ev_set_pgmchange(&this->ev, channel, program);
    sendEvent(true);
}

void InterfaceAlsa::setVolumeChangeEvent(int channel, int volume)
{
    qDebug() << "channel: "+QString::number(channel)+" volume: "+QString::number(volume);
    snd_seq_ev_set_controller(&ev, channel, MIDI_CTL_LSB_MAIN_VOLUME, volume);
    
    sendEvent(true);
}

void InterfaceAlsa::setAttackChanged(int channel, int value)
{
    qDebug() << "attack"+QString::number(value);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_SC4_ATTACK_TIME, value);
    
    sendEvent(true);
}

void InterfaceAlsa::setReleaseChanged(int channel, int value)
{
    qDebug() << "release"+QString::number(value);
    snd_seq_ev_set_controller(&this->ev, channel, MIDI_CTL_SC3_RELEASE_TIME, value);
    
    sendEvent(true);
}

void InterfaceAlsa::sendEvent(bool drain)
{
    snd_seq_ev_set_direct(&this->ev);
    //snd_seq_ev_set_source(&ev, my_port);
    //snd_seq_ev_set_dest(&ev, seq_client, seq_port);
    
    snd_seq_event_output(this->seq, &this->ev);
    
    if (drain)
    {
        snd_seq_drain_output(this->seq);
    }
}
