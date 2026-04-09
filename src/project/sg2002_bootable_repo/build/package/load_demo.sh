#!/bin/sh
set -eu
mkdir -p /mnt/data
cp bootloader.bin kernel.bin worker.bin mars_mcu_fw.bin 8051_boot_cfg.ini manifest.json /mnt/data/
echo 'Load bootloader.bin to 0x80080000'
echo 'Load kernel.bin to 0x80100000'
echo 'Load worker.bin to 0x80180000'
echo 'Load mars_mcu_fw.bin with the vendor 8051 DDR path'
echo 'Jump to bootloader.bin; it will start the 8051 and hand off to kernel.bin'
