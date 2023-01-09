#!/bin/bash

rm -r build
mkdir build

gcc src/*.c -lraylib -lm -o build/ld52

build/ld52