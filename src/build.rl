#!/usr/local/bin/earl -xe

module Build

$"cc -ggdb -o main main.c $(pkg-config --libs --cflags ncurses)";
