#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>


#define MAX_MIDI_BUFFER 1024  // temporary per-process buffer
#define NUM_CHANNELS 16

jack_port_t *midi_in;
jack_port_t *midi_out;
jack_client_t *client;
jack_ringbuffer_t *midi_rb;

uint8_t volume[NUM_CHANNELS]; // per-channel volume 0-127

// Simple MIDI scaling for Note On velocity
static void scale_note_velocity(uint8_t *event, size_t size) {
    if (size < 3) return;
    uint8_t status = event[0] & 0xF0;
    uint8_t channel = event[0] & 0x0F;
    if (status == 0x90 && event[2] > 0) { // Note On with velocity > 0
        event[2] = (event[2] * volume[channel]) / 127;
    }
}

// JACK process callback
int process(jack_nframes_t nframes, void *arg) {
    void *inbuf = jack_port_get_buffer(midi_in, nframes);
    void *outbuf = jack_port_get_buffer(midi_out, nframes);
    jack_midi_clear_buffer(outbuf);

    jack_nframes_t events = jack_midi_get_event_count(inbuf);
    for (jack_nframes_t i = 0; i < events; i++) {
        jack_midi_event_t event;
        jack_midi_event_get(&event, inbuf, i);

        // Copy event to temp buffer and scale velocity
        uint8_t tmp[MAX_MIDI_BUFFER];
        if (event.size > MAX_MIDI_BUFFER) continue;
        memcpy(tmp, event.buffer, event.size);
        scale_note_velocity(tmp, event.size);

        // Write to output
        jack_midi_event_write(outbuf, event.time, tmp, event.size);

        // Optionally write to ringbuffer for other threads
        if (jack_ringbuffer_write_space(midi_rb) >= event.size + 1) {
            jack_ringbuffer_write(midi_rb, tmp, event.size);
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    const char *client_name = "midi_forwarder";
    jack_options_t options = JackNullOption;
    jack_status_t status;

    client = jack_client_open(client_name, options, &status, NULL);
    if (!client) {
        fprintf(stderr, "Failed to open JACK client\n");
        return 1;
    }

    // Initialize volumes
    for (int i = 0; i < NUM_CHANNELS; i++) volume[i] = 127; // max volume

    // Register ports
    midi_in = jack_port_register(client, "input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    midi_out = jack_port_register(client, "output", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

    // Create ringbuffer
    midi_rb = jack_ringbuffer_create(4096);
    if (!midi_rb) {
        fprintf(stderr, "Failed to create ringbuffer\n");
        return 1;
    }

    jack_set_process_callback(client, process, 0);

    if (jack_activate(client)) {
        fprintf(stderr, "Cannot activate client\n");
        return 1;
    }

    printf("MIDI forwarding active with per-channel volume.\n");

    // Example: external thread could read from midi_rb
    while (1) sleep(1);

    jack_client_close(client);
    jack_ringbuffer_free(midi_rb);
    return 0;
}

