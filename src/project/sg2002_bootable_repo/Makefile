CROSS ?= riscv64-unknown-elf-
CC := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump
SIZE := $(CROSS)size
SDCC ?= sdcc
PACKIHX ?= packihx
PYTHON ?= python3
MAKE ?= make

RV_MARCH ?= rv64imac
RV_MABI ?= lp64
EXTRA_CFLAGS ?=
RV_CFLAGS := -march=$(RV_MARCH) -mabi=$(RV_MABI) -mcmodel=medany -ffreestanding -fno-builtin -fno-pic -fno-omit-frame-pointer -Wall -Wextra -Iinclude -Ishared -Ithird_party/tinyusb/src -Os $(EXTRA_CFLAGS)
RV_LDFLAGS := -nostdlib -nostartfiles -Wl,--build-id=none
OUT ?= build
TINYUSB_SRCS := third_party/tinyusb/src/tusb.c third_party/tinyusb/src/class/cdc/cdc_device.c third_party/tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c
COMMON_SRCS := shared/uart.c shared/timer.c shared/mailbox.c shared/cache.c shared/c906_cache_ops.c shared/memory.c shared/shared_state.c shared/usb_serial.c shared/usb_descriptors.c shared/usb_dwc2_board.c shared/worker_release.c shared/worker_reset_sg200x.c shared/image.c shared/platform.c shared/trap.c shared/trap_entry.S shared/riscv_identity.c shared/riscv_csrs.S $(TINYUSB_SRCS)
HEADERS := $(shell find include third_party/tinyusb/src -name '*.h' -print)
BOOT_SRCS := bootloader/start.S bootloader/main.c $(COMMON_SRCS)
KERNEL_SRCS := kernel/start.S kernel/main.c $(COMMON_SRCS)
WORKER_SRCS := worker_rtos/start.S worker_rtos/main.c $(COMMON_SRCS)

all: $(OUT)/bootloader.bin $(OUT)/kernel.bin $(OUT)/worker.bin $(OUT)/mars_mcu_fw.bin package

$(OUT):
	mkdir -p $(OUT)

$(OUT)/bootloader.elf: $(BOOT_SRCS) $(HEADERS) bootloader/linker.ld | $(OUT)
	$(CC) $(RV_CFLAGS) $(RV_LDFLAGS) -T bootloader/linker.ld -o $@ $(BOOT_SRCS)
	$(SIZE) $@

$(OUT)/kernel.elf: $(KERNEL_SRCS) $(HEADERS) kernel/linker.ld | $(OUT)
	$(CC) $(RV_CFLAGS) $(RV_LDFLAGS) -T kernel/linker.ld -o $@ $(KERNEL_SRCS)
	$(SIZE) $@

$(OUT)/worker.elf: $(WORKER_SRCS) $(HEADERS) worker_rtos/linker.ld | $(OUT)
	$(CC) $(RV_CFLAGS) $(RV_LDFLAGS) -T worker_rtos/linker.ld -o $@ $(WORKER_SRCS)
	$(SIZE) $@

$(OUT)/bootloader.bin: $(OUT)/bootloader.elf
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -D $< > $(OUT)/bootloader.dis

$(OUT)/kernel.bin: $(OUT)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -D $< > $(OUT)/kernel.dis

$(OUT)/worker.bin: $(OUT)/worker.elf
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -D $< > $(OUT)/worker.dis

$(OUT)/mars_mcu_fw.bin: watch8051/watchdog.c watch8051/Makefile | $(OUT)
	$(MAKE) -C watch8051 OUT=../$(OUT) SDCC=$(SDCC) PACKIHX=$(PACKIHX)

package: $(OUT)/bootloader.bin $(OUT)/kernel.bin $(OUT)/worker.bin $(OUT)/mars_mcu_fw.bin
	$(PYTHON) tools/package_images.py --bootloader $(OUT)/bootloader.bin --kernel $(OUT)/kernel.bin --worker $(OUT)/worker.bin --mcu $(OUT)/mars_mcu_fw.bin --kraken-header include/kraken.h --outdir $(OUT)/package

clean:
	rm -rf $(OUT)
	$(MAKE) -C watch8051 clean OUT=../$(OUT)

.PHONY: all clean package
