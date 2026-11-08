#include "interface_jack.h"

/* Updated to use JACK ring buffer */

InterfaceJack::InterfaceJack(int id, InterfaceAudio *parent)
    : InterfaceAudio(id, parent), client(nullptr), ringBuffer(nullptr)
{
    char client_name[64];
    snprintf(client_name, sizeof(client_name),
             "vkeybd-qt-jack-%d", id);
    
    client = jack_client_open(client_name, JackNullOption, nullptr);
    if (!client) {
        qWarning() << "Failed to open JACK client";
        return;
    }
    
    // Create pre-allocated lock-free ring buffer (128 KB)
    ringBuffer = jack_ringbuffer_create(128 * 1024);
    if (!ringBuffer) {
        qWarning() << "Failed to create JACK ring buffer";
        return;
    }
    
    jack_set_process_callback(client, jackCallback, this);
    
    if (jack_activate(client)) {
        qWarning() << "Cannot activate JACK client";
    }
}

InterfaceJack::~InterfaceJack()
{
    if (client) {
        jack_deactivate(client);
        jack_client_close(client);
    }
    if (ringBuffer) {
        jack_ringbuffer_free(ringBuffer);
        ringBuffer = nullptr;
    }
}

void InterfaceJack::createNewPort(QString label)
{
    if (!client) return;
    
    const int index = this->output_port_count.load(std::memory_order_relaxed);
    if (index >= MAX_PORTS) {
        qWarning() << "Too many JACK ports, not registering" << label;
        return;
    }
    
    jack_port_t* port = jack_port_register(client,
                                           label.toUtf8(),
                                           JACK_DEFAULT_MIDI_TYPE,
                                           JackPortIsOutput,
                                           0);
    if (!port)
        qWarning() << "Failed to register JACK port" << label;
    
    // the slot is taken even if registration failed, so slot index == tab id stays true
    this->output_ports[index] = port;
    this->output_port_count.store(index + 1, std::memory_order_release);
}

// Push event into JACK ring buffer (lock-free)
void InterfaceJack::pushEvent(const MidiEvent &event)
{
    if (!ringBuffer) return;
    
    // Never write a partial event: the reader would be out of step from then on
    if (jack_ringbuffer_write_space(ringBuffer) < sizeof(MidiEvent)) {
        qWarning() << "Ring buffer overflow, MIDI event lost!";
        return;
    }
    jack_ringbuffer_write(ringBuffer, (const char*)&event, sizeof(MidiEvent));
}

// JACK callback: called in real-time audio thread
int InterfaceJack::jackCallback(jack_nframes_t nframes, void *arg)
{
    InterfaceJack *self = static_cast<InterfaceJack*>(arg);
    self->processRingBuffer(nframes);
    return 0;
}

// Process events from ring buffer and send to JACK ports
void InterfaceJack::processRingBuffer(jack_nframes_t nframes)
{
    const int port_count = this->output_port_count.load(std::memory_order_acquire);
    for (int i = 0; i < port_count; ++i) {
        if (!this->output_ports[i])
            continue;
        void *buf = jack_port_get_buffer(this->output_ports[i], nframes);
        jack_midi_clear_buffer(buf);
    }
    
    if (!ringBuffer) return;
    
    MidiEvent ev;
    // Read as many complete events as possible
    while (jack_ringbuffer_read_space(ringBuffer) >= sizeof(MidiEvent)) {
        jack_ringbuffer_read(ringBuffer, (char*)&ev, sizeof(MidiEvent));
        
        if (ev.port < 0 || ev.port >= port_count || !this->output_ports[ev.port]) continue;
        
        void *buf = jack_port_get_buffer(this->output_ports[ev.port], nframes);
        unsigned char midi[3];
        
        switch (ev.type) {
        case 0: // Note On
            midi[0] = 0x90 | (ev.channel & 0x0F);
            midi[1] = ev.data1 & 0x7F;
            midi[2] = ev.data2 & 0x7F;
            jack_midi_event_write(buf, 0, midi, 3);
            break;
        case 1: // Note Off
            midi[0] = 0x80 | (ev.channel & 0x0F);
            midi[1] = ev.data1 & 0x7F;
            midi[2] = ev.data2 & 0x7F;
            jack_midi_event_write(buf, 0, midi, 3);
            break;
        case 2: // CC
            midi[0] = 0xB0 | (ev.channel & 0x0F);
            midi[1] = ev.data1 & 0x7F;
            midi[2] = ev.data2 & 0x7F;
            jack_midi_event_write(buf, 0, midi, 3);
            break;
        case 3: // Program Change
            midi[0] = 0xC0 | (ev.channel & 0x0F);
            midi[1] = ev.data1 & 0x7F;
            jack_midi_event_write(buf, 0, midi, 2);
            break;
        case 4: // Pitch Bend
            midi[0] = 0xE0 | (ev.channel & 0x0F); // status byte
            midi[1] = ev.data1 & 0x7F;           // LSB
            midi[2] = ev.data2 & 0x7F;           // MSB
            jack_midi_event_write(buf, 0, midi, 3);
            break;
        }
    }
}

// Event handlers remain unchanged
void InterfaceJack::keyPressEvent(int port, int channel, uint8_t midicode, uint8_t velocity)
{
    pushEvent({port, channel, MidiEvent::NOTE_ON, midicode, velocity});
}

void InterfaceJack::keyReleaseEvent(int port, int channel, uint8_t midicode, uint8_t velocity)
{
    pushEvent({port, channel, MidiEvent::NOTE_OFF, midicode, velocity});
}

void InterfaceJack::keyPanicEvent(int port, int channel)
{
    // CC 123: All Notes Off (128 single note-offs per channel and tab flooded the buffer)
    pushEvent({port, channel, MidiEvent::CC, 123, 0});
}

void InterfaceJack::keyStopAllEvent(int port, int channel)
{
    // CC 120: All Sound Off, once is enough
    pushEvent({port, channel, MidiEvent::CC, 120, 0});
}

void InterfaceJack::keyPitchbendEvent(int port, int channel, int pitch)
{
    // Clamp pitch to 0–16383 (14-bit)
    if (pitch < 0) pitch = 0;
    if (pitch > 16383) pitch = 16383;
    
    // Split into LSB / MSB for 14-bit pitch bend
    uint8_t lsb = pitch & 0x7F;
    uint8_t msb = (pitch >> 7) & 0x7F;
    
    // Type 4 = pitch bend
    pushEvent({port, channel, MidiEvent::PITCH_BEND, lsb, msb});
}


void InterfaceJack::keySustainEvent(int port, int channel, bool pressed)
{
    pushEvent({port, channel, MidiEvent::CC, 64, pressed ? MIDI_ON : MIDI_OFF});
}

void InterfaceJack::keySostenutoEvent(int port, int channel, bool pressed)
{
    pushEvent({port, channel, MidiEvent::CC, 66, pressed ? MIDI_ON : MIDI_OFF});
}

void InterfaceJack::keySoftEvent(int port, int channel, bool pressed)
{
    pushEvent({port, channel, MidiEvent::CC, 67, pressed ? MIDI_ON : MIDI_OFF});
}

void InterfaceJack::setProgramChangeEvent(int port, int channel, uint8_t program, int bank)
{
    if (bank < 0) bank = 0;
    
    // Bank MSB (CC0)
    uint8_t bankMSB = (bank >> 7) & 0x7F;
    pushEvent({port, channel, MidiEvent::CC, 0, bankMSB});
    
    // Bank LSB (CC32)
    uint8_t bankLSB = bank & 0x7F;
    pushEvent({port, channel, MidiEvent::CC, 32, bankLSB});
    
    // Program Change
    pushEvent({port, channel, MidiEvent::PROGRAM, program, 0});
}

void InterfaceJack::setControlChangeEvent(int port, int channel, uint8_t cc, uint8_t value)
{
    // Clamp to valid MIDI range just to be safe
    cc &= 0x7F;
    value &= 0x7F;
    
    // CC event (type = 2)
    pushEvent({port, channel, MidiEvent::CC, cc, value});
}

void InterfaceJack::setVolumeChangeEvent(int port, int channel, uint8_t volume)
{
    pushEvent({port, channel, MidiEvent::CC, 7, volume});
}

void InterfaceJack::setPanChangeEvent(int port, int channel, uint8_t value)
{
    pushEvent({port, channel, MidiEvent::CC, 10, value});
}

void InterfaceJack::setPortamentoChanged(int port, int channel, uint8_t value)
{
    pushEvent({port, channel, MidiEvent::CC, 5, value});
}

void InterfaceJack::setAttackChanged(int port, int channel, uint8_t value)
{
    pushEvent({port, channel, MidiEvent::CC, 73, value});
}

void InterfaceJack::setReleaseChanged(int port, int channel, uint8_t value)
{
    pushEvent({port, channel, MidiEvent::CC, 72, value});
}

void InterfaceJack::setTremoloChanged(int port, int channel, uint8_t value)
{
    pushEvent({port, channel, MidiEvent::CC, 1, value});
}
