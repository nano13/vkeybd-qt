#ifndef INTERFACEALSA_H
#define INTERFACEALSA_H

#include <QObject>
#include <QDebug>

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
    
    void createNewPort(QString label);
    
    void keyPressEvent(int port, int channel, uint8_t midicode, uint8_t velocity);
    void keyReleaseEvent(int port, int channel, uint8_t midicode, uint8_t velocity);
    void keyPanicEvent(int port, int channel);
    void keyStopAllEvent(int port, int channel);
    void keyPitchbendEvent(int port, int channel, int pitch);
    void keySustainEvent(int port, int channel, bool pressed);
    void keySostenutoEvent(int port, int channel, bool pressed);
    void keySoftEvent(int port, int channel, bool pressed);
    void setProgramChangeEvent(int port, int channel, uint8_t program, int bank);
    void setControlChangeEvent(int port, int channel, uint8_t cc, uint8_t value);
    void setVolumeChangeEvent(int port, int channel, uint8_t volume);
    void setPanChangeEvent(int port, int channel, uint8_t value);
    void setPortamentoChanged(int port, int channel, uint8_t value);
    void setAttackChanged(int port, int channel, uint8_t value);
    void setReleaseChanged(int port, int channel, uint8_t value);
    void setTremoloChanged(int port, int channel, uint8_t value);
    
    QString NAME = "vkeybd-qt";
    
private:
    snd_seq_t *seq;
    snd_seq_event_t ev;
    
    void sendEvent(bool drain);
    
signals:
    
};

#endif // INTERFACEALSA_H
