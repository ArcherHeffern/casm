#!/usr/bin/env bash 

set -euo pipefail

gcc -o casm casm.c preprocess.c lexer.c util.c
