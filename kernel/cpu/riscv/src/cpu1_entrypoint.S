
# SPDX-License-Identifier: MIT

#include <config.h>

	# The global pointer is used for relaxation of `.data` accesses.
	.global __global_pointer$



#if __riscv_xlen == 64
#define ST_REG sd
#define LD_REG ld
#else
#define ST_REG sw
#define LD_REG lw
#endif



	# Entrypoint from the bootloader.
	.text
	.align 2
	.global _cpu1_start
	.type _cpu1_start, %function
_cpu1_start:
	# Set up registers.
	.option push
	.option norelax
	la   gp, __global_pointer$
	.option pop
	mv   tp, x0
	LD_REG sp, cpu1_temp_stack
	
	# Jump to the C entrypoint.
	jal  cpu1_init
	# This function isn't allowed to return.
	ebreak
