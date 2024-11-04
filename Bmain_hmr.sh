#!/usr/bin/env bash

set -euo pipefail

LIBS='-Wl,-rpath,./raylib-5.0_macos/lib -I./raylib-5.0_macos/include -L./raylib-5.0_macos/lib -lraylib -lm'
WARNINGS='-Wall -Wextra'
gcc -fPIC -shared -o libplug.so plug.c util.c $LIBS
gcc $WARNINGS -o main_hmr main_hmr.c $LIBS
