#!/bin/sh
set -eu
SRC_DIR=${1:-./build/package}
OUT=${2:-./sg2002-board-bundle.tar.gz}
[ -d "$SRC_DIR" ] || { echo "missing source dir: $SRC_DIR" >&2; exit 1; }
tar -C "$SRC_DIR" -czf "$OUT" .
echo "$OUT"
