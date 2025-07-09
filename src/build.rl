#!/usr/local/bin/earl -xe

module Build

$"cc -o main main.c $(pkg-config --libs --cflags ncurses)";
