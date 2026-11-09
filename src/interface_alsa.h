#ifndef INTERFACEALSA_H
#define INTERFACEALSA_H

#include <QObject>
#include <QDebug>
#include <QSocketNotifier>
#include <QList>

#include "interface_audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include <alsa/seq.h>

/*
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include </usr/include/alsa/asoundlib.h>
#include </usr/include/alsa/pcm.h>
*/

class InterfaceAlsa : public InterfaceAudio
{
    Q_OBJECT
public:
    explicit InterfaceAlsa(int id = 0, InterfaceAudio *parent = nullptr);
    ~InterfaceAlsa();
    
    void createNewPort(QString label) override;
    
    void keyPressEvent(int port, int channel, uint8_t midicode, uint8_t velocity) override;
    void keyReleaseEvent(int port, int channel, uint8_t midicode, uint8_t velocity) override;
    void keyPanicEvent(int port, int channel) override;
    void keyStopAllEvent(int port, int channel) override;
    void keyPitchbendEvent(int port, int channel, int pitch) override;
    void keySustainEvent(int port, int channel, bool pressed) override;
    void keySostenutoEvent(int port, int channel, bool pressed) override;
    void keySoftEvent(int port, int channel, bool pressed) override;
    void setProgramChangeEvent(int port, int channel, uint8_t program, int bank) override;
    void setControlChangeEvent(int port, int channel, uint8_t cc, uint8_t value) override;
    void setVolumeChangeEvent(int port, int channel, uint8_t volume) override;
    void setPanChangeEvent(int port, int channel, uint8_t value) override;
    void setPortamentoChanged(int port, int channel, uint8_t value) override;
    void setAttackChanged(int port, int channel, uint8_t value) override;
    void setReleaseChanged(int port, int channel, uint8_t value) override;
    void setTremoloChanged(int port, int channel, uint8_t value) override;
    
    void createMidiInput(QString label) override;
    
    QString NAME = "vkeybd-qt";
    
private:
    snd_seq_t *seq = nullptr;     // nullptr if the sequencer could not be opened
    snd_seq_event_t ev;
    
    void sendEvent(bool drain);
    
    // MIDI input: an own sequencer handle, opened non-blocking and read in the
    // GUI thread whenever its poll descriptors become readable
    snd_seq_t *seq_in = nullptr;
    QList<QSocketNotifier*> input_notifiers;
    void readMidiInput();
    
signals:
    
};

#endif // INTERFACEALSA_H
