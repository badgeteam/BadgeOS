
/* SPDX-License-Identifier: MIT */

OUTPUT_FORMAT("elf32-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
	xip    (RX)  : ORIGIN = 0x42000000, LENGTH = 16M
	sram   (RWX) : ORIGIN = 0x40800000, LENGTH = 512K
	lpsram (RWX) : ORIGIN = 0x50000000, LENGTH = 16K
}

PHDRS
{
	codeseg    PT_LOAD;
	rodataseg  PT_LOAD;
	ramcodeseg PT_LOAD;
	dataseg    PT_LOAD;
}

SECTIONS
{
	/DISCARD/ : { *(.note) *(.note.*) *(.eh_frame) *(.eh_frame.*) }

	/* Import hardware addresses. */
INCLUDE esp32c6.peripherals.ld
INCLUDE esp32c6.rom.ld
INCLUDE esp32c6.rom.api.ld
PROVIDE(INTPRI = 0x600C5000);
PROVIDE(PLIC_MX = 0x20001000);
PROVIDE(PLIC_UX = 0x20001400);
PROVIDE(CLINT_M = 0x20001800);
PROVIDE(CLINT_U = 0x20001C00);

	/* Physical memory. */
	__start_xip    = 0x42000000;
	__stop_xip     = 0x43000000;
	__start_sram   = 0x40800000;
	__stop_sram    = 0x40880000;
	__start_lpsram = 0x50000000;
	__stop_lpsram  = 0x50004000;
	
	__section_alignment = 16;
	
	/* ESP image header. */
	. = __start_xip;
	.esphdr : AT(0) {
		BYTE(0xE9);			/* Magic byte. */
		BYTE(3);			/* Segment count. */
		BYTE(0x02);			/* SPI mode. */
		BYTE(0x1f);			/* SPI speed/size. */
		LONG(_start);		/* Entrypoint. */
		BYTE(0xee);			/* WP pin state. */
		BYTE(0x00);			/* Drive settings. */
		BYTE(0x00);
		BYTE(0x00);
		SHORT(0x000D);		/* Chip (ESP32-C6). */
		BYTE(0x00);			/* (deprecated) */
		SHORT(0x0000);		/* Min chip rev. */
		SHORT(0xffff);		/* Max chip rev. */
		LONG(0x00000000);	/* (reserved) */
		BYTE(0x00);			/* SHA256 appended (not appended). */
	} >xip :codeseg
	
	/* ESP image segment 0: code. */
	.espseg.0 : AT(LOADADDR(.esphdr) + SIZEOF(.esphdr)) {
		LONG(__start_text);
		LONG(__stop_text - __start_text);
	} >xip :codeseg
	
	__start_text = .;
	.text : AT(LOADADDR(.espseg.0) + SIZEOF(.espseg.0)) {
		. = ALIGN(256);
		*(.interrupt_vector_table)
		*(.text) *(.text.*)
		. = ALIGN(4);
	} >xip :codeseg
	__stop_text = .;
	
	/* ESP image segment 1: rodata. */
	.espseg.1 : AT(LOADADDR(.text) + SIZEOF(.text)) {
		LONG(__start_rodata);
		LONG(__stop_rodata - __start_rodata);
	} >xip :codeseg
	
	__start_rodata = .;
	.got : AT(LOADADDR(.espseg.1) + SIZEOF(.espseg.1)) {
		*(.got.plt) *(.igot.plt) *(.got) *(.igot)
	} >xip :rodataseg
	.plt : AT(LOADADDR(.got) + SIZEOF(.got)) {
		*(.plt) *(.iplt)
	} >xip :rodataseg
	.rodata : AT(LOADADDR(.plt) + SIZEOF(.plt)) {
		. = ALIGN(__section_alignment);
		*(.rodata) *(.rodata.*)
        __start_fsdrivers = .;
        KEEP(*(.fsdrivers .fsdrivers.*))
        __stop_fsdrivers = .;
		. = ALIGN(4);
	} >xip :rodataseg
	__stop_rodata = .;
	
	/* ESP image segment 2: data. */
	.espseg.2 : AT(LOADADDR(.rodata) + SIZEOF(.rodata)) {
		LONG(__start_data);
		LONG(__stop_data - __start_data);
	} >xip :rodataseg
	
	. = __start_sram;
	__start_data = .;
	.ramtext : AT(LOADADDR(.espseg.2) + SIZEOF(.espseg.2)) {
		. = ALIGN(__section_alignment);
		*(.ramtext) *(.ramtext.*)
		. = ALIGN(__section_alignment);
	} >sram :ramcodeseg
	.data : AT(LOADADDR(.ramtext) + SIZEOF(.ramtext)) {
		. = ALIGN(__section_alignment);
		*(.data) *(.data.*)
		. = ALIGN(__section_alignment);
	} >sram :dataseg
	__start_ss = .;
	.sdata : AT(LOADADDR(.data) + SIZEOF(.data)) {
		*(.sdata) *(.sdata.*)
		*(.srodata) *(.srodata.*)
		. = ALIGN(4);
	} >sram :dataseg
	__stop_data = .;
	
	/* Zero-initialized */
	__start_bss = .;
	.sbss : {
		. = ALIGN(__section_alignment);
		*(.sbss) *(.sbss.*)
	} >sram :NONE
	__stop_ss = .;
	.bss : {
		. = ALIGN(__section_alignment);
		*(.bss) *(.bss.*) *(COMMON)
	} >sram :NONE
	__stop_bss = .;
	PROVIDE( __global_pointer$ = (__stop_ss + __start_ss) / 2 );
	
	/* Uninitialized */
	. = ALIGN(__section_alignment);
	__start_noinit = .;
	.noinit : {
		*(.noinit) *(.noinit.*)
	} >sram :NONE
	__stop_noinit = .;
	
	/* Unallocated physical memory. */
	__start_free_xip    = (__stop_rodata + 0xffff) & 0xffff0000;
	__stop_free_xip     = __stop_xip;
	__start_free_sram   = (__stop_bss + 0xf) & 0xfffffff0;
	__stop_free_sram    = __stop_sram;
	__start_free_lpsram = __start_lpsram;
	__stop_free_lpsram  = __stop_lpsram;
}
