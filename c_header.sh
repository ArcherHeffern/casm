#!/usr/bin/env bash

set -euo pipefail

ggrep -P '^\w+\**\s+\w+\([^)]*\)\s*{' $1 | sed -Ee 's/ *{/\;/'
