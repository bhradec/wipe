#!/bin/sh

mkdir -p ./build/
gcc -ansi -Wall -pedantic-errors -g wipe.c -o ./build/wipe
