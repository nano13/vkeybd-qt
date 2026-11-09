#include "interface_audio.h"

InterfaceAudio::InterfaceAudio(int id, QObject *parent)
    : QObject(parent), id(id)
{
    
}

InterfaceAudio::~InterfaceAudio()
{
    
}

void InterfaceAudio::createNewPort(QString label)
{
    Q_UNUSED(label);
}

QString InterfaceAudio::label()
{
    return "";
}

void InterfaceAudio::keyPressEvent(int port, int channel, uint8_t midicode, uint8_t velocity)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(midicode);
    Q_UNUSED(velocity);
}

void InterfaceAudio::keyReleaseEvent(int port, int channel, uint8_t midicode, uint8_t velocity)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(midicode);
    Q_UNUSED(velocity);
}

void InterfaceAudio::keyPanicEvent(int port, int channel)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
}

void InterfaceAudio::keyStopAllEvent(int port, int channel)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
}

void InterfaceAudio::keyPitchbendEvent(int port, int channel, int pitch)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(pitch);
}

void InterfaceAudio::keySustainEvent(int port, int channel, bool pressed)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(pressed);
}

void InterfaceAudio::keySostenutoEvent(int port, int channel, bool pressed)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(pressed);
}

void InterfaceAudio::keySoftEvent(int port, int channel, bool pressed)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(pressed);
}

void InterfaceAudio::setProgramChangeEvent(int port, int channel, uint8_t program, int bank)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(program);
    Q_UNUSED(bank);
}

void InterfaceAudio::setControlChangeEvent(int port, int channel, uint8_t cc, uint8_t value)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(cc);
    Q_UNUSED(value);
}

void InterfaceAudio::setVolumeChangeEvent(int port, int channel, uint8_t volume)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(volume);
}

void InterfaceAudio::setPanChangeEvent(int port, int channel, uint8_t value)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(value);
}

void InterfaceAudio::setPortamentoChanged(int port, int channel, uint8_t value)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(value);
}

void InterfaceAudio::setAttackChanged(int port, int channel, uint8_t value)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(value);
}

void InterfaceAudio::setReleaseChanged(int port, int channel, uint8_t value)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(value);
}

void InterfaceAudio::setTremoloChanged(int port, int channel, uint8_t value)
{
    Q_UNUSED(port);
    Q_UNUSED(channel);
    Q_UNUSED(value);
}


void InterfaceAudio::createMidiInput(QString label)
{
    Q_UNUSED(label);
}

void InterfaceAudio::dispatchMidiMessage(uint8_t status, uint8_t data1, uint8_t data2)
{
    data1 &= 0x7F;
    data2 &= 0x7F;
    
    switch (status & 0xF0)
    {
    case 0x90:
        // note on with velocity 0 is a note off
        if (data2 == 0)
            emit signalMidiNoteOff(data1);
        else
            emit signalMidiNoteOn(data1, data2);
        break;
    case 0x80:
        emit signalMidiNoteOff(data1);
        break;
    case 0xB0:
        emit signalMidiControlChange(data1, data2);
        break;
    case 0xE0:
        emit signalMidiPitchBend(data1 | (data2 << 7));
        break;
    default:
        // program change, aftertouch, system messages: not used
        break;
    }
}
