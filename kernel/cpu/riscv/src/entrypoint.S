
# SPDX-License-Identifier: MIT

	# This is the C entrypoint, which will be called when basic CRT is set up.
	.global basic_runtime_init
	# The global pointer is used for relaxation of `.data` accesses.
	.global __global_pointer$
	# First address in `.bss` (inclusive).
	.global __start_bss
	# Last address in `.bss` (exclusive).
	.global __stop_bss
	# First address in `.init_array` (inclusive).
	.global __start_init_array
	# Last address in `.init_array` (exclusive).
	.global __stop_init_array



	# Reserve stack.
	.section ".bss"
	.align 4
	.global __stack_top
	.global __stack_bottom
	.global __stack_size
	.equ __stack_size, 8192
	.lcomm __stack_bottom, __stack_size
	.equ __stack_top, __stack_bottom + __stack_size



	# Entrypoint from the bootloader.
	.text
	.align 2
	.global _start
	.type _start, %function
_start:
	# Set up registers.
	.option push
	.option norelax
	la gp, __global_pointer$
	mv tp, x0
	la sp, __stack_top
	.option pop
	
	# Zero out .bss section.
	la a0, __start_bss
	la a1, __stop_bss
	beq a0, a1, .bssinit_skip
.bssinit_loop:
	sw x0, 0(a0)
	addi a0, a0, 4
	bne a0, a1, .bssinit_loop
.bssinit_skip:
	
	# Run init functions.
	la s0, __start_init_array
	la s1, __stop_init_array
	beq s0, s1, .initfun_skip
.initfun_loop:
	lw ra, 0(s0)
	jalr ra, ra
	addi s0, s0, 4
	bne s0, s1, .initfun_loop
.initfun_skip:
	
	# Jump to the C entrypoint.
	jal basic_runtime_init
	# This function isn't allowed to return.
	ebreak
