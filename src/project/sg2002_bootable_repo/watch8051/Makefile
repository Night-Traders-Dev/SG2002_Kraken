OUT ?= ../build
SDCC ?= sdcc
PACKIHX ?= packihx
all: $(OUT)/mars_mcu_fw.bin
$(OUT)/mars_mcu_fw.ihx: watchdog.c | $(OUT)
	$(SDCC) -mmcs51 --model-small --iram-size 256 --xram-size 0x2000 --code-size 0x2000 --xram-loc 0x0000 --code-loc 0x0000 -o $@ $<
$(OUT)/mars_mcu_fw.bin: $(OUT)/mars_mcu_fw.ihx
	$(PACKIHX) $< > $(OUT)/mars_mcu_fw.hex
	objcopy -I ihex -O binary $(OUT)/mars_mcu_fw.hex $@
$(OUT):
	mkdir -p $(OUT)
clean:
	rm -f $(OUT)/mars_mcu_fw.ihx $(OUT)/mars_mcu_fw.hex $(OUT)/mars_mcu_fw.bin
.PHONY: all clean
