#!/bin/bash
gcc play.c `sdl-config --cflags --libs` -g -lm -o play
