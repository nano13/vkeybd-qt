#!/usr/bin/env python3

import jack

client = jack.Client("MIDI_Listener")
midi_in = client.midi_inports.register("input")

# This callback is called by JACK in the real-time thread
@client.set_process_callback
def process(frames):
    for offset, midi_data in midi_in.incoming_midi_events():
        status = midi_data[0]
        data1 = midi_data[1] if len(midi_data) > 1 else None
        data2 = midi_data[2] if len(midi_data) > 2 else None
        print(f"MIDI: status={status}, data1={data1}, data2={data2}")

# Activate client after registering ports
client.activate()

# Auto-connect physical MIDI output ports
for port in client.get_ports(is_physical=True, is_output=True):
    try:
        client.connect(port, midi_in)
        print(f"Connected {port} → MIDI_Listener:input")
    except jack.JackError:
        pass

print("Listening for MIDI input. Press Ctrl+C to exit.")

# Keep the script alive (JACK calls process automatically)
try:
    import time
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\nExiting.")
    client.deactivate()

