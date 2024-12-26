
# SPDX-License-Identifier: MIT

#include <config.h>

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



	# Entrypoint from the bootloader.
	.text
	.global _start
	.type _start, %function
_start:
