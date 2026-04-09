#!/bin/sh
set -eu
G=/sys/kernel/config/usb_gadget/cvitek
[ -d "$G" ] || exit 0
if [ -e "$G/UDC" ]; then
  echo '' > "$G/UDC" || true
fi
rm -f "$G/configs/c.1/acm.usb0" || true
rmdir "$G/functions/acm.usb0" 2>/dev/null || true
rmdir "$G/configs/c.1/strings/0x409" 2>/dev/null || true
rmdir "$G/configs/c.1" 2>/dev/null || true
rmdir "$G/strings/0x409" 2>/dev/null || true
rmdir "$G" 2>/dev/null || true
rm -f /boot/usb.device || true
: > /boot/usb.host
