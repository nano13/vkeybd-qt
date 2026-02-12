#ifndef INTERFACEAUDIO_H
#define INTERFACEAUDIO_H

#include <QObject>
#include <QMap>

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
    
    virtual void saveMIDISettings();
    virtual void loadMIDISettings();
    
protected:
    int id;
    
signals:
    
};

#endif // INTERFACEAUDIO_H
