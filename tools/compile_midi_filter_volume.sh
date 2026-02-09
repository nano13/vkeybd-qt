#!/bin/bash

gcc midi_filter_volume.c -o midi_filter_volume `pkg-config --cflags --libs jack`

