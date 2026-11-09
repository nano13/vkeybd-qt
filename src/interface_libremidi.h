#ifndef INTERFACE_LIBREMIDI_H
#define INTERFACE_LIBREMIDI_H

#include <QObject>
#include <QString>

#include <memory>
#include <vector>

#include "interface_audio.h"

class QTimer;

namespace libremidi
{
class midi_in;
class midi_out;
}

/*
 * MIDI backend based on libremidi (https://github.com/celtera/libremidi).
 *
 * On Linux the API is chosen once for the whole application, in this order:
 * JACK (also covers PipeWire through pipewire-jack), then ALSA sequencer.
 * Every keyboard gets ONE client of that API (like the native JACK and ALSA
 * backends): one output port per tab plus the keyboard's MIDI input port.
 * On other platforms libremidi's default API is used (CoreMIDI on macOS, ...),
 * with one libremidi object per port.
 *
 * The libremidi headers are only included in the .cpp file.
 */
class InterfaceLibremidi : public InterfaceAudio
{
    Q_OBJECT
public:
    explicit InterfaceLibremidi(int id = 0, QObject *parent = nullptr);
    ~InterfaceLibremidi() override;

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

    // defined in the .cpp file
    struct SharedClient;
    struct InputQueue;

private:
    struct Output
    {
        std::unique_ptr<libremidi::midi_out> midi_out;
        bool open = false;
    };

    // the client shared by all ports of this keyboard; must outlive them
    std::unique_ptr<SharedClient> shared;
    // index = port = tab id; a slot is kept even if opening failed
    std::vector<Output> outputs;
    std::unique_ptr<libremidi::midi_in> input;

    // incoming bytes: written on the libremidi thread, read on the GUI thread
    std::unique_ptr<InputQueue> input_queue;
    QTimer *input_timer = nullptr;

    // set when the client could not be created or a port failed: nothing more is attempted
    bool failed = false;

    bool ensureClient();
    void drainMidiInput();

    void send(int port, uint8_t status, uint8_t data1, uint8_t data2);
    void send(int port, uint8_t status, uint8_t data1);
    void sendControl(int port, int channel, uint8_t cc, uint8_t value);
};

#endif // INTERFACE_LIBREMIDI_H
