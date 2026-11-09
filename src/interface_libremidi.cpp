#include "interface_libremidi.h"

#include <QDebug>
#include <QTimer>

#include <libremidi/libremidi.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <string>
#include <string_view>
#include <thread>
#include <variant>
#include <vector>

#if defined(__linux__)
#include <alsa/asoundlib.h>
#include <jack/jack.h>
#endif

namespace
{
constexpr uint8_t NOTE_OFF = 0x80;
constexpr uint8_t NOTE_ON = 0x90;
constexpr uint8_t CONTROL_CHANGE = 0xB0;
constexpr uint8_t PROGRAM_CHANGE = 0xC0;
constexpr uint8_t PITCH_BEND = 0xE0;

constexpr uint8_t CC_BANK_MSB = 0;
constexpr uint8_t CC_PORTAMENTO_TIME = 5;
constexpr uint8_t CC_VOLUME = 7;
constexpr uint8_t CC_PAN = 10;
constexpr uint8_t CC_BANK_LSB = 32;
constexpr uint8_t CC_SUSTAIN = 64;
constexpr uint8_t CC_PORTAMENTO = 65;
constexpr uint8_t CC_SOSTENUTO = 66;
constexpr uint8_t CC_SOFT = 67;
constexpr uint8_t CC_RELEASE = 72;
constexpr uint8_t CC_ATTACK = 73;
constexpr uint8_t CC_TREMOLO_DEPTH = 92;
constexpr uint8_t CC_ALL_SOUND_OFF = 120;
constexpr uint8_t CC_ALL_NOTES_OFF = 123;

QString errorText(const stdx::error &error)
{
    // stdx::string_ref is not null-terminated: use data() and size()
    const stdx::string_ref message = error.message();
    return QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size()));
}

QString apiName(libremidi::API api)
{
    const std::string_view name = libremidi::get_api_name(api);
    return QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()));
}

bool apiCompiled(libremidi::API api)
{
    const std::vector<libremidi::API> available = libremidi::available_apis();
    for (const libremidi::API compiled : available)
    {
        if (compiled == api)
            return true;
    }
    return false;
}

// sets client_name on every backend configuration that has one
struct SetClientName
{
    std::string name;
    
    template<typename Configuration>
    void operator()(Configuration &configuration) const
    {
        if constexpr (requires { configuration.client_name = this->name; })
            configuration.client_name = this->name;
    }
};

// API chosen by the first keyboard, used by all keyboards (-1 = not chosen yet)
int chosen_api = -1;
}

#if defined(__linux__)
/*
 * One JACK client shared by all libremidi ports of a keyboard (see
 * examples/jack_share.cpp in libremidi). libremidi registers one process
 * callback per port; our JACK process callback runs them.
 *
 * Callbacks are only added on the GUI thread, into a fixed array that the
 * JACK thread reads up to an atomic count, so no locking in the RT thread.
 */
class JackShared
{
public:
    static constexpr int MAX_CALLBACKS = 512;
    
    jack_client_t *client = nullptr;
    
    explicit JackShared(const std::string &name)
    {
        jack_status_t status = static_cast<jack_status_t>(0);
        this->client = jack_client_open(name.c_str(), JackNoStartServer, &status);
        if (!this->client)
            return;
        
        jack_set_process_callback(this->client, &JackShared::process, this);
        if (jack_activate(this->client) != 0)
        {
            jack_client_close(this->client);
            this->client = nullptr;
        }
    }
    
    ~JackShared()
    {
        if (this->client)
        {
            jack_deactivate(this->client);
            jack_client_close(this->client);
        }
    }
    
    void add(libremidi::jack_callback callback)
    {
        const int index = this->count.load(std::memory_order_relaxed);
        if (index >= MAX_CALLBACKS)
        {
            qWarning() << "libremidi: too many JACK ports in one client";
            return;
        }
        this->callback_slots[index].callback = std::move(callback);
        this->callback_slots[index].alive.store(true);
        this->count.store(index + 1, std::memory_order_release);
    }
    
    void remove(int64_t token)
    {
        const int count = this->count.load(std::memory_order_relaxed);
        for (int index = 0; index < count; index++)
        {
            if (this->callback_slots[index].alive.load() && this->callback_slots[index].callback.token == token)
            {
                this->callback_slots[index].alive.store(false);
                // the JACK thread may still be inside this callback: wait for the cycle to end
                waitForCycleEnd();
                return;
            }
        }
    }
    
    // lets queued messages go out before the ports are closed
    void waitCycles(int cycles)
    {
        const uint64_t start = this->cycle_count.load();
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
        while (this->cycle_count.load() < start + static_cast<uint64_t>(cycles) && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
    
private:
    struct Slot
    {
        std::atomic<bool> alive{false};
        libremidi::jack_callback callback;
    };
    
    std::array<Slot, MAX_CALLBACKS> callback_slots;
    std::atomic<int> count{0};
    std::atomic<bool> in_cycle{false};
    std::atomic<uint64_t> cycle_count{0};
    
    void waitForCycleEnd()
    {
        // pairs with process(): in_cycle is set before alive is read there
        if (!this->in_cycle.load())
            return;
        const uint64_t start = this->cycle_count.load();
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while (this->cycle_count.load() == start && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    static int process(jack_nframes_t nframes, void *arg)
    {
        JackShared *shared = static_cast<JackShared*>(arg);
        shared->in_cycle.store(true);
        
        const int count = shared->count.load(std::memory_order_acquire);
        for (int index = 0; index < count; index++)
        {
            if (shared->callback_slots[index].alive.load())
                shared->callback_slots[index].callback.callback(static_cast<int>(nframes));
        }
        
        shared->in_cycle.store(false);
        shared->cycle_count.fetch_add(1);
        return 0;
    }
};
#endif

/*
 * Shared client of one keyboard. For JACK and ALSA all ports live in it;
 * for other APIs it only carries the API, and libremidi opens one client
 * per object.
 */
struct InterfaceLibremidi::SharedClient
{
    libremidi::API api = libremidi::API::UNSPECIFIED;
    std::string client_name;
#if defined(__linux__)
    std::unique_ptr<JackShared> jack;
    snd_seq_t *seq = nullptr;
#endif
    
    ~SharedClient()
    {
        // the ports are gone by now (see ~InterfaceLibremidi)
#if defined(__linux__)
        this->jack.reset();
        if (this->seq)
            snd_seq_close(this->seq);
#endif
    }
    
    libremidi::output_api_configuration outputConfiguration()
    {
#if defined(__linux__)
        if (this->jack)
        {
            JackShared *jack_shared = this->jack.get();
            return libremidi::jack_output_configuration{
                .client_name = this->client_name,
                .context = jack_shared->client,
                .set_process_func = [jack_shared](libremidi::jack_callback callback) { jack_shared->add(std::move(callback)); },
                .clear_process_func = [jack_shared](int64_t token) { jack_shared->remove(token); },
            };
        }
        if (this->seq)
        {
            return libremidi::alsa_seq::output_configuration{
                .client_name = this->client_name,
                .context = this->seq,
            };
        }
#endif
        libremidi::output_api_configuration configuration = libremidi::midi_out_configuration_for(this->api);
        std::visit(SetClientName{this->client_name}, configuration);
        return configuration;
    }
    
    libremidi::input_api_configuration inputConfiguration()
    {
#if defined(__linux__)
        if (this->jack)
        {
            JackShared *jack_shared = this->jack.get();
            return libremidi::jack_input_configuration{
                .client_name = this->client_name,
                .context = jack_shared->client,
                .set_process_func = [jack_shared](libremidi::jack_callback callback) { jack_shared->add(std::move(callback)); },
                .clear_process_func = [jack_shared](int64_t token) { jack_shared->remove(token); },
            };
        }
#endif
        // ALSA: a shared input would need our own poll loop, so the input
        // gets its own sequencer client (one per keyboard)
        libremidi::input_api_configuration configuration = libremidi::midi_in_configuration_for(this->api);
        std::visit(SetClientName{this->client_name + "-in"}, configuration);
        return configuration;
    }
};

/*
 * Single producer (libremidi input thread), single consumer (GUI thread).
 * Holds status | data1 << 8 | data2 << 16.
 */
struct InterfaceLibremidi::InputQueue
{
    static constexpr int SIZE = 1024;
    
    std::array<uint32_t, SIZE> data{};
    std::atomic<int> head{0};
    std::atomic<int> tail{0};
    
    void push(uint32_t value)
    {
        const int head = this->head.load(std::memory_order_relaxed);
        const int next = (head + 1) % SIZE;
        if (next == this->tail.load(std::memory_order_acquire))
            return; // full: drop
        this->data[head] = value;
        this->head.store(next, std::memory_order_release);
    }
    
    bool pop(uint32_t &value)
    {
        const int tail = this->tail.load(std::memory_order_relaxed);
        if (tail == this->head.load(std::memory_order_acquire))
            return false;
        value = this->data[tail];
        this->tail.store((tail + 1) % SIZE, std::memory_order_release);
        return true;
    }
};

InterfaceLibremidi::InterfaceLibremidi(int id, QObject *parent)
    : InterfaceAudio(id, parent)
{
    
}

InterfaceLibremidi::~InterfaceLibremidi()
{
    for (int port = 0; port < static_cast<int>(this->outputs.size()); port++)
    {
        for (int channel = 0; channel < 16; channel++)
            sendControl(port, channel, CC_ALL_NOTES_OFF, 0);
    }
    
#if defined(__linux__)
    // JACK sends queued messages in the next cycles only
    if (this->shared && this->shared->jack)
        this->shared->jack->waitCycles(2);
#endif
    
    // ports first (they unregister from the shared client), then the client
    if (this->input_timer)
        this->input_timer->stop();
    this->input.reset();
    this->outputs.clear();
    this->shared.reset();
}

bool InterfaceLibremidi::ensureClient()
{
    if (this->shared)
        return true;
    if (this->failed)
        return false;
    
    std::unique_ptr<SharedClient> shared = std::make_unique<SharedClient>();
    shared->client_name = "vkeybd-qt-" + std::to_string(this->id + 1);
    
#if defined(__linux__)
    // JACK first, then ALSA; once chosen, all keyboards use the same API
    const bool try_jack = (chosen_api < 0 || chosen_api == static_cast<int>(libremidi::API::JACK_MIDI));
    const bool try_alsa = (chosen_api < 0 || chosen_api == static_cast<int>(libremidi::API::ALSA_SEQ));
    
    if (try_jack && apiCompiled(libremidi::API::JACK_MIDI))
    {
        std::unique_ptr<JackShared> jack = std::make_unique<JackShared>(shared->client_name);
        if (jack->client)
        {
            // JACK may have renamed the client if the name was taken
            shared->client_name = jack_get_client_name(jack->client);
            shared->jack = std::move(jack);
            shared->api = libremidi::API::JACK_MIDI;
        }
        else
        {
            qWarning() << "libremidi: cannot open JACK client" << QString::fromStdString(shared->client_name);
        }
    }
    if (!shared->jack && try_alsa && apiCompiled(libremidi::API::ALSA_SEQ))
    {
        snd_seq_t *seq = nullptr;
        const int error = snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
        if (error >= 0)
        {
            snd_seq_set_client_name(seq, shared->client_name.c_str());
            shared->seq = seq;
            shared->api = libremidi::API::ALSA_SEQ;
        }
        else
        {
            qWarning() << "libremidi: cannot open the ALSA sequencer:" << snd_strerror(error);
        }
    }
    if (!shared->jack && !shared->seq)
    {
        qWarning() << "libremidi: no usable MIDI API, MIDI disabled for keyboard" << (this->id + 1);
        this->failed = true;
        return false;
    }
#else
    shared->api = libremidi::midi1::default_api();
#endif
    
    if (chosen_api < 0)
    {
        chosen_api = static_cast<int>(shared->api);
        qInfo() << "libremidi: using" << apiName(shared->api);
    }
    
    this->shared = std::move(shared);
    return true;
}

void InterfaceLibremidi::createNewPort(QString label)
{
    // a slot is always added, so port numbers keep matching the tab ids
    Output output;
    
    if (!this->failed && ensureClient())
    {
        try
        {
            output.midi_out = std::make_unique<libremidi::midi_out>(libremidi::output_configuration{}, this->shared->outputConfiguration());
            stdx::error error = output.midi_out->open_virtual_port(label.toStdString());
            if (error != stdx::error{})
            {
                qWarning() << "libremidi:" << apiName(this->shared->api) << "cannot open virtual output port" << label << ":" << errorText(error);
                output.midi_out.reset();
            }
        }
        catch (const std::exception &exception)
        {
            qWarning() << "libremidi:" << apiName(this->shared->api) << "cannot open virtual output port" << label << ":" << exception.what();
            output.midi_out.reset();
        }
        
        if (!output.midi_out)
        {
            qWarning() << "libremidi: giving up on further ports of keyboard" << (this->id + 1);
            this->failed = true;
        }
    }
    
    output.open = (output.midi_out != nullptr);
    this->outputs.push_back(std::move(output));
}

void InterfaceLibremidi::createMidiInput(QString label)
{
    if (this->input || this->failed || !ensureClient())
        return;
    
    this->input_queue = std::make_unique<InputQueue>();
    InputQueue *queue = this->input_queue.get();
    
    libremidi::input_configuration configuration;
    configuration.on_message = [queue](const libremidi::message &message) {
        // called on a libremidi thread (the JACK RT thread with JACK): no allocations, no Qt
        if (message.size() < 2 || message.size() > 3)
            return;
        
        const uint32_t status = message[0];
        const uint32_t data1 = message[1];
        const uint32_t data2 = (message.size() > 2) ? message[2] : 0;
        queue->push(status | (data1 << 8) | (data2 << 16));
    };
    
    try
    {
        this->input = std::make_unique<libremidi::midi_in>(configuration, this->shared->inputConfiguration());
        stdx::error error = this->input->open_virtual_port(label.toStdString());
        if (error != stdx::error{})
        {
            qWarning() << "libremidi:" << apiName(this->shared->api) << "cannot open virtual input port" << label << ":" << errorText(error);
            this->input.reset();
        }
    }
    catch (const std::exception &exception)
    {
        qWarning() << "libremidi:" << apiName(this->shared->api) << "cannot open virtual input port" << label << ":" << exception.what();
        this->input.reset();
    }
    
    if (!this->input)
        return;
    
    this->input_timer = new QTimer(this);
    connect(this->input_timer, &QTimer::timeout, this, &InterfaceLibremidi::drainMidiInput);
    this->input_timer->start(2);
}

void InterfaceLibremidi::drainMidiInput()
{
    uint32_t value = 0;
    while (this->input_queue->pop(value))
        dispatchMidiMessage(static_cast<uint8_t>(value & 0xFF), static_cast<uint8_t>((value >> 8) & 0xFF), static_cast<uint8_t>((value >> 16) & 0xFF));
}

void InterfaceLibremidi::send(int port, uint8_t status, uint8_t data1, uint8_t data2)
{
    if (port < 0 || port >= static_cast<int>(this->outputs.size()) || !this->outputs[port].open)
        return;
    
    stdx::error error = this->outputs[port].midi_out->send_message(status, static_cast<uint8_t>(data1 & 0x7F), static_cast<uint8_t>(data2 & 0x7F));
    if (error != stdx::error{})
        qWarning() << "libremidi: send failed:" << errorText(error);
}

void InterfaceLibremidi::send(int port, uint8_t status, uint8_t data1)
{
    if (port < 0 || port >= static_cast<int>(this->outputs.size()) || !this->outputs[port].open)
        return;
    
    stdx::error error = this->outputs[port].midi_out->send_message(status, static_cast<uint8_t>(data1 & 0x7F));
    if (error != stdx::error{})
        qWarning() << "libremidi: send failed:" << errorText(error);
}

void InterfaceLibremidi::sendControl(int port, int channel, uint8_t cc, uint8_t value)
{
    send(port, static_cast<uint8_t>(CONTROL_CHANGE | (channel & 0x0F)), cc, value);
}

void InterfaceLibremidi::keyPressEvent(int port, int channel, uint8_t midicode, uint8_t velocity)
{
    send(port, static_cast<uint8_t>(NOTE_ON | (channel & 0x0F)), midicode, velocity);
}

void InterfaceLibremidi::keyReleaseEvent(int port, int channel, uint8_t midicode, uint8_t velocity)
{
    send(port, static_cast<uint8_t>(NOTE_OFF | (channel & 0x0F)), midicode, velocity);
}

void InterfaceLibremidi::keyPanicEvent(int port, int channel)
{
    sendControl(port, channel, CC_ALL_NOTES_OFF, 0);
}

void InterfaceLibremidi::keyStopAllEvent(int port, int channel)
{
    sendControl(port, channel, CC_ALL_SOUND_OFF, 0);
}

void InterfaceLibremidi::keyPitchbendEvent(int port, int channel, int pitch)
{
    // 0..16383, center 8192 (as for the JACK backend)
    pitch = qBound(0, pitch, 16383);
    send(port, static_cast<uint8_t>(PITCH_BEND | (channel & 0x0F)), static_cast<uint8_t>(pitch & 0x7F), static_cast<uint8_t>((pitch >> 7) & 0x7F));
}

void InterfaceLibremidi::keySustainEvent(int port, int channel, bool pressed)
{
    sendControl(port, channel, CC_SUSTAIN, pressed ? 127 : 0);
}

void InterfaceLibremidi::keySostenutoEvent(int port, int channel, bool pressed)
{
    sendControl(port, channel, CC_SOSTENUTO, pressed ? 127 : 0);
}

void InterfaceLibremidi::keySoftEvent(int port, int channel, bool pressed)
{
    sendControl(port, channel, CC_SOFT, pressed ? 127 : 0);
}

void InterfaceLibremidi::setProgramChangeEvent(int port, int channel, uint8_t program, int bank)
{
    if (bank < 0)
        bank = 0;
    
    sendControl(port, channel, CC_BANK_MSB, static_cast<uint8_t>((bank >> 7) & 0x7F));
    sendControl(port, channel, CC_BANK_LSB, static_cast<uint8_t>(bank & 0x7F));
    send(port, static_cast<uint8_t>(PROGRAM_CHANGE | (channel & 0x0F)), program);
}

void InterfaceLibremidi::setControlChangeEvent(int port, int channel, uint8_t cc, uint8_t value)
{
    sendControl(port, channel, cc, value);
}

void InterfaceLibremidi::setVolumeChangeEvent(int port, int channel, uint8_t volume)
{
    sendControl(port, channel, CC_VOLUME, volume);
}

void InterfaceLibremidi::setPanChangeEvent(int port, int channel, uint8_t value)
{
    sendControl(port, channel, CC_PAN, value);
}

void InterfaceLibremidi::setPortamentoChanged(int port, int channel, uint8_t value)
{
    // same as the ALSA backend: switch (CC 65) plus time (CC 5)
    sendControl(port, channel, CC_PORTAMENTO, value > 0 ? 127 : 0);
    sendControl(port, channel, CC_PORTAMENTO_TIME, value);
}

void InterfaceLibremidi::setAttackChanged(int port, int channel, uint8_t value)
{
    sendControl(port, channel, CC_ATTACK, value);
}

void InterfaceLibremidi::setReleaseChanged(int port, int channel, uint8_t value)
{
    sendControl(port, channel, CC_RELEASE, value);
}

void InterfaceLibremidi::setTremoloChanged(int port, int channel, uint8_t value)
{
    sendControl(port, channel, CC_TREMOLO_DEPTH, value);
}
