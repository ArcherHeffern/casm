#!/usr/bin/env bash

set -euo pipefail

LIBS='-Wl,-rpath,./raylib-5.0_macos/lib -I./raylib-5.0_macos/include -L./raylib-5.0_macos/lib -lraylib -lm'
WARNINGS='-Wall -Wextra'
gcc $WARNINGS -o main main.c ui.c util.c casm.c preprocess.c lexer.c animation.c complex_animations.c future.c render.c style_overrides.c $LIBS
