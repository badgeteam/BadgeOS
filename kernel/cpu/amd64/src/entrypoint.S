
# SPDX-License-Identifier: MIT

#include <config.h>

	# This is the C entrypoint, which will be called when basic CRT is set up.
	.global basic_runtime_init



	# Entrypoint from the bootloader.
	.text
	.global _start
	.type _start, %function
	.cfi_startproc
_start:
	.intel_syntax
	.cfi_undefined %rsp
	.cfi_undefined %rbp
	jmp basic_runtime_init
	.cfi_endproc
