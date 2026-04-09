# Kraken SG2002 full bundle manifest

This bundle provides an integrated U-Boot bring-up script for the new four-image
layout. It stages the bootloader, kernel, worker, and 8051 images into DDR and
then jumps into the bootloader.

For the LicheeRV Nano W RISC-V SD boot path, `src/build.sh` now also emits a
vendor-style `boot.sd` FIT payload. That path is separate from this script
bundle: `boot.sd` is meant for vendor U-Boot's automatic `bootm` flow, while
`boot_sg2002_full.scr` remains the manual staging/debug path.
