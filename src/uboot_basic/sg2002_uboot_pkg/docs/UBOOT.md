# U-Boot deployment flow for Kraken SG2002

This package provides a U-Boot script template that stages `bootloader.bin`,
`kernel.bin`, `worker.bin`, and `mars_mcu_fw.bin` from MMC into RAM, clears the
shared page, and jumps into the bootloader image.

The kernel, not U-Boot, now owns secondary-core release. That keeps worker
bring-up inside the custom OS instead of baking policy into the boot script.
