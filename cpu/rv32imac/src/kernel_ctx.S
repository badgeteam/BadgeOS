
# SPDX-License-Identifier: MIT

#include "cpu/regs.h"
	.global _kernel_reg_dump



	# Print a register dump of the current registers.
	.global kernel_cur_regs_dump
	.type kernel_cur_regs_dump, %function
kernel_cur_regs_dump:
	# Save all registers to stack.
	addi sp, sp, -128
	sw ra, cpu_regs_t_pc(sp) # PC inferred.
	sw x0, cpu_regs_t_ra(sp) # RA omitted.
	# sw sp, cpu_regs_t_sp(sp) # SP inferred.
	sw gp, cpu_regs_t_gp(sp)
	sw tp, cpu_regs_t_tp(sp)
	sw t0, cpu_regs_t_t0(sp)
	sw t1, cpu_regs_t_t1(sp)
	sw t2, cpu_regs_t_t2(sp)
	sw s0, cpu_regs_t_s0(sp)
	sw s1, cpu_regs_t_s1(sp)
	sw a0, cpu_regs_t_a0(sp)
	sw a1, cpu_regs_t_a1(sp)
	sw a2, cpu_regs_t_a2(sp)
	sw a3, cpu_regs_t_a3(sp)
	sw a4, cpu_regs_t_a4(sp)
	sw a5, cpu_regs_t_a5(sp)
	sw a6, cpu_regs_t_a6(sp)
	sw a7, cpu_regs_t_a7(sp)
	sw s2, cpu_regs_t_s2(sp)
	sw s3, cpu_regs_t_s3(sp)
	sw s4, cpu_regs_t_s4(sp)
	sw s5, cpu_regs_t_s5(sp)
	sw s6, cpu_regs_t_s6(sp)
	sw s7, cpu_regs_t_s7(sp)
	sw s8, cpu_regs_t_s8(sp)
	sw s9, cpu_regs_t_s9(sp)
	sw s10, cpu_regs_t_s10(sp)
	sw s11, cpu_regs_t_s11(sp)
	sw t3, cpu_regs_t_t3(sp)
	sw t4, cpu_regs_t_t4(sp)
	sw t5, cpu_regs_t_t5(sp)
	sw t6, cpu_regs_t_t6(sp)
	# Store the real value of SP and call the printer.
	addi a0, sp, 128
	sw a0, cpu_regs_t_sp(sp)
	mv a0, sp
	jal _kernel_reg_dump
	# Restore all registers.
	lw ra, cpu_regs_t_pc(sp) # PC inferred.
	# lw ra, cpu_regs_t_ra(sp) # RA omitted.
	# lw sp, cpu_regs_t_sp(sp) # SP inferred.
	lw gp, cpu_regs_t_gp(sp)
	lw tp, cpu_regs_t_tp(sp)
	lw t0, cpu_regs_t_t0(sp)
	lw t1, cpu_regs_t_t1(sp)
	lw t2, cpu_regs_t_t2(sp)
	lw s0, cpu_regs_t_s0(sp)
	lw s1, cpu_regs_t_s1(sp)
	lw a0, cpu_regs_t_a0(sp)
	lw a1, cpu_regs_t_a1(sp)
	lw a2, cpu_regs_t_a2(sp)
	lw a3, cpu_regs_t_a3(sp)
	lw a4, cpu_regs_t_a4(sp)
	lw a5, cpu_regs_t_a5(sp)
	lw a6, cpu_regs_t_a6(sp)
	lw a7, cpu_regs_t_a7(sp)
	lw s2, cpu_regs_t_s2(sp)
	lw s3, cpu_regs_t_s3(sp)
	lw s4, cpu_regs_t_s4(sp)
	lw s5, cpu_regs_t_s5(sp)
	lw s6, cpu_regs_t_s6(sp)
	lw s7, cpu_regs_t_s7(sp)
	lw s8, cpu_regs_t_s8(sp)
	lw s9, cpu_regs_t_s9(sp)
	lw s10, cpu_regs_t_s10(sp)
	lw s11, cpu_regs_t_s11(sp)
	lw t3, cpu_regs_t_t3(sp)
	lw t4, cpu_regs_t_t4(sp)
	lw t5, cpu_regs_t_t5(sp)
	lw t6, cpu_regs_t_t6(sp)
	# Return.
	addi sp, sp, 128
	ret
	