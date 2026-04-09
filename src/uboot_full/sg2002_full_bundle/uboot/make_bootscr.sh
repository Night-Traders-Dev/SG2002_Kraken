#!/bin/sh
set -eu
INPUT=${1:-boot_sg2002_full.txt}
OUTPUT=${2:-boot_sg2002_full.scr}
MKIMAGE=${MKIMAGE:-mkimage}
"$MKIMAGE" -T script -n "SG2002 integrated boot" -C none -d "$INPUT" "$OUTPUT"
echo "$OUTPUT"
