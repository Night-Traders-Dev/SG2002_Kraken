#!/bin/sh
set -eu
INPUT=${1:-boot_sg2002.txt}
OUTPUT=${2:-boot_sg2002.scr}
MKIMAGE=${MKIMAGE:-mkimage}
"$MKIMAGE" -T script -n "SG2002 custom boot" -C none -d "$INPUT" "$OUTPUT"
echo "$OUTPUT"
