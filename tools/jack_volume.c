#include <jack/jack.h>
#include <jack/midiport.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

jack_port_t *inputL, *inputR;
jack_port_t *outputL, *outputR;
jack_port_t *midi_in;
jack_client_t *client;

float target_volume = 1.0f;   // volume from MIDI
float current_volume = 1.0f;  // actual applied volume
const float smoothing = 0.001f; // smaller = smoother

// Process callback with soft clipping and smooth volume
int process(jack_nframes_t nframes, void *arg) {
    jack_default_audio_sample_t *in_l = jack_port_get_buffer(inputL, nframes);
    jack_default_audio_sample_t *in_r = jack_port_get_buffer(inputR, nframes);
    jack_default_audio_sample_t *out_l = jack_port_get_buffer(outputL, nframes);
    jack_default_audio_sample_t *out_r = jack_port_get_buffer(outputR, nframes);

    // MIDI handling
    void *midi_buf = jack_port_get_buffer(midi_in, nframes);
    jack_midi_event_t ev;
    int n_events = jack_midi_get_event_count(midi_buf);

    for (int i = 0; i < n_events; i++) {
        if (jack_midi_event_get(&ev, midi_buf, i) == 0 && ev.size >= 3) {
            uint8_t status = ev.buffer[0] & 0xF0;
            uint8_t cc = ev.buffer[1];
            uint8_t value = ev.buffer[2];
            if (status == 0xB0 && cc == 7) { // CC7 = volume
                float max_gain = 3.0f; // up to 3x boost
                target_volume = (value / 127.0f) * max_gain;
            }
        }
    }

    // Apply volume with smoothing + soft clipping
    for (jack_nframes_t i = 0; i < nframes; i++) {
        current_volume += (target_volume - current_volume) * smoothing;

        // amplify
        float l = in_l[i] * current_volume;
        float r = in_r[i] * current_volume;

        // soft clip using tanh (gentle saturation)
        out_l[i] = tanhf(l);
        out_r[i] = tanhf(r);
    }

    return 0;
}


// JACK shutdown callback
void shutdown(void *arg) {
    fprintf(stderr, "JACK shutdown!\n");
}

int main(int argc, char *argv[]) {
    const char *default_name = "jack_volume_control";
    const char *client_name = default_name;

    // Parse command-line arguments for -l
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            client_name = argv[i + 1];
            i++; // skip next argument since we used it
        }
    }

    jack_options_t options = JackNullOption;
    jack_status_t status;

    // Open client
    client = jack_client_open(client_name, options, &status);
    if (!client) {
        fprintf(stderr, "Cannot open JACK client\n");
        return 1;
    }

    jack_set_process_callback(client, process, 0);
    jack_on_shutdown(client, shutdown, 0);

    // Register audio ports
    inputL = jack_port_register(client, "inL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    inputR = jack_port_register(client, "inR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    outputL = jack_port_register(client, "outL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    outputR = jack_port_register(client, "outR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    // Register MIDI port
    midi_in = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

    if (jack_activate(client)) {
        fprintf(stderr, "Cannot activate JACK client\n");
        return 1;
    }

    printf("JACK client '%s' active. Listening for CC7 (volume) MIDI messages...\n", client_name);

    // Auto-connect system capture to input and output to system playback
    const char **ports;
    ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);
    if (ports) {
        //jack_connect(client, ports[0], "jack_volume_control:inL");
        //jack_connect(client, ports[1], "jack_volume_control:inR");
        //free(ports);
    }
    ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
    if (ports) {
        jack_connect(client, "jack_volume_control:outL", ports[0]);
        jack_connect(client, "jack_volume_control:outR", ports[1]);
        free(ports);
    }

    // Keep running
    while (1) sleep(1);

    jack_client_close(client);
    return 0;
}
