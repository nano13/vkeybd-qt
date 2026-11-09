#ifndef INTERFACEAUDIO_H
#define INTERFACEAUDIO_H

#include <QObject>
#include <QMap>

#include <cstdint>

class InterfaceAudio : public QObject
{
    Q_OBJECT
public:
    explicit InterfaceAudio(int id = 0, QObject *parent = nullptr);
    ~InterfaceAudio();
    
    virtual QString label();
    
    virtual void createNewPort(QString label);
    
    virtual void keyPressEvent(int port, int channel, uint8_t midicode, uint8_t velocity);
    virtual void keyReleaseEvent(int port, int channel, uint8_t midicode, uint8_t velocity);
    virtual void keyPanicEvent(int port, int channel);
    virtual void keyStopAllEvent(int port, int channel);
    virtual void keyPitchbendEvent(int port, int channel, int pitch);
    virtual void keySustainEvent(int port, int channel, bool pressed);
    virtual void keySostenutoEvent(int port, int channel, bool pressed);
    virtual void keySoftEvent(int port, int channel, bool pressed);
    virtual void setProgramChangeEvent(int port, int channel, uint8_t program, int bank);
    virtual void setControlChangeEvent(int port, int channel, uint8_t cc, uint8_t value);
    virtual void setVolumeChangeEvent(int port, int channel, uint8_t volume);
    virtual void setPanChangeEvent(int port, int channel, uint8_t value);
    virtual void setPortamentoChanged(int port, int channel, uint8_t value);
    virtual void setAttackChanged(int port, int channel, uint8_t value);
    virtual void setReleaseChanged(int port, int channel, uint8_t value);
    virtual void setTremoloChanged(int port, int channel, uint8_t value);
    
    // MIDI input of this keyboard instance. Messages of all MIDI channels are
    // accepted and reported with the signals below.
    virtual void createMidiInput(QString label);
    
    // decodes one channel voice message and emits the matching signal
    void dispatchMidiMessage(uint8_t status, uint8_t data1, uint8_t data2);
    
protected:
    int id;
    
signals:
    void signalMidiNoteOn(int note, int velocity);
    void signalMidiNoteOff(int note);
    void signalMidiControlChange(int cc, int value);
    // 0..16383, center 8192
    void signalMidiPitchBend(int value);
};

#endif // INTERFACEAUDIO_H
