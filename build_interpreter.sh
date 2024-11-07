#!/usr/bin/env bash
set -euo pipefail
LIBS='-Wl,-rpath,./raylib-5.0_macos/lib -I./raylib-5.0_macos/include -L./raylib-5.0_macos/lib -lraylib -lm'

SRC="src"

WARNINGS='-Wall -Wextra'
gcc $WARNINGS -o interpreter $SRC/interpreter_main.c $SRC/interpreter.c $SRC/util.c $SRC/casm.c $SRC/preprocess.c $SRC/lexer.c $LIBS