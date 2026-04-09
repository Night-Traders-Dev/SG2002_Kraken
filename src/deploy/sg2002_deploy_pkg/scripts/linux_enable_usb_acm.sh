#!/bin/sh
set -eu

G=/sys/kernel/config/usb_gadget/cvitek
UDC=${UDC:-40e0000.cvi-usb-dev}
VID=${VID:-0x30b1}
PID=${PID:-0x1003}
SERIAL=${SERIAL:-0123456789}
MANUFACTURER=${MANUFACTURER:-Cvitek}
PRODUCT=${PRODUCT:-USB Com Port}

modprobe libcomposite 2>/dev/null || true
mountpoint -q /sys/kernel/config || mount -t configfs none /sys/kernel/config

if [ -e /boot/usb.host ]; then
  rm -f /boot/usb.host
fi
: > /boot/usb.device

mkdir -p "$G"
echo "$VID" > "$G/idVendor"
echo "$PID" > "$G/idProduct"
mkdir -p "$G/strings/0x409"
echo "$SERIAL" > "$G/strings/0x409/serialnumber"
echo "$MANUFACTURER" > "$G/strings/0x409/manufacturer"
echo "$PRODUCT" > "$G/strings/0x409/product"
mkdir -p "$G/configs/c.1/strings/0x409"
echo config1 > "$G/configs/c.1/strings/0x409/configuration"
echo 120 > "$G/configs/c.1/MaxPower"
mkdir -p "$G/functions/acm.usb0"
ln -sf "$G/functions/acm.usb0" "$G/configs/c.1/acm.usb0"
echo "$UDC" > "$G/UDC"
echo 'USB CDC ACM gadget enabled. Connect host and open /dev/ttyACM*.'
