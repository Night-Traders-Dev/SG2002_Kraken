OUTPUT_ARCH(riscv)
ENTRY(_start)
MEMORY { ram (rwx) : ORIGIN = 0x80280000, LENGTH = 256K }
SECTIONS {
  . = ORIGIN(ram);
  .text : { KEEP(*(.text.start)) *(.text*) *(.rodata*) } > ram
  .data : { *(.data*) } > ram
  .bss (NOLOAD) : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; } > ram
  . = ALIGN(16); . += 12K; __stack_top = .;
}
