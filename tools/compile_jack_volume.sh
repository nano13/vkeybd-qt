#!/bin/bash

gcc -o jack_volume jack_volume.c $(pkg-config --cflags --libs jack) -lm

