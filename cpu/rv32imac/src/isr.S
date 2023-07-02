
# SPDX-License-Identifier: MIT

#include "cpu/isr.h"
	.global __global_pointer$



	# Entry code for an ISR or trap handler; save tempregs and swap out SP/GP/TP.
	# Assumes valid `kernel_ctx_t *` containing a valid `cpu_regs_t *` in CSR mscratch.
	.macro isr_entry
	# Save tempregs t0-t3.
	csrrw t0, mscratch, t0
	sw t1, kernel_ctx_t_scratch1(t0)
	csrrw t1, mscratch, t0
	sw t1, kernel_ctx_t_scratch0(t0)
	lw t1, kernel_ctx_t_regs(t0)
	
	sw t2, cpu_regs_t_t2(t1)
	sw t3, cpu_regs_t_t3(t1)
	
	sw ra, cpu_regs_t_ra(t1)
	sw sp, cpu_regs_t_sp(t1)
	sw gp, cpu_regs_t_gp(t1)
	sw tp, cpu_regs_t_tp(t1)
	
	# Clear context to switch field.
	sw x0, kernel_ctx_t_ctxswitch(t0)
	
	# Move tempregs.
	lw t2, kernel_ctx_t_scratch0(t0)
	sw t2, cpu_regs_t_t0(t1)
	lw t2, kernel_ctx_t_scratch1(t0)
	sw t2, cpu_regs_t_t1(t1)
	
	# Save PC.
	csrr t2, mepc
	sw t2, cpu_regs_t_pc(t1)
	
	# Set up special regs.
	li tp, 0
	.option push
	.option norvc
	la gp, __global_pointer$
	.option pop
	la sp, __interrupt_stack_hi
	.endm



	# Exit code for an ISR or trap handler; restores tempregs and SP/GP/TP.
	# Assumes `kernel_ctx_t *` in t0 and `cpu_regs_t *` in t1.
	.macro isr_exit
	# Restore PC.
	lw t2, cpu_regs_t_pc(t1)
	csrw mepc, t2
	
	lw ra, cpu_regs_t_ra(t1)
	lw sp, cpu_regs_t_sp(t1)
	lw gp, cpu_regs_t_gp(t1)
	lw tp, cpu_regs_t_tp(t1)
	
	# Restore tempregs t0-t3.
	lw t2, cpu_regs_t_t0(t1)
	csrw mscratch, t2
	lw t3, cpu_regs_t_t3(t1)
	lw t2, cpu_regs_t_t2(t1)
	lw t1, cpu_regs_t_t1(t1)
	csrrw t0, mscratch, t0
	.endm



	# Load `kernel_ctx_t *` from CSR mscratch into t0 and load `cpu_regs_t *` into t1.
	.macro get_kctx
	csrr t0, mscratch
	lw t1, kernel_ctx_t_regs(t0)
	.endm



	# Save all regs not saved by `isr_entry`.
	# Assumes `kernel_ctx_t *` in t0 and `cpu_regs_t *` in t1.
	.macro save_regs
	# Save all regs.
	sw s0, cpu_regs_t_s0(t1)
	sw s1, cpu_regs_t_s1(t1)
	sw a0, cpu_regs_t_a0(t1)
	sw a1, cpu_regs_t_a1(t1)
	sw a2, cpu_regs_t_a2(t1)
	sw a3, cpu_regs_t_a3(t1)
	sw a4, cpu_regs_t_a4(t1)
	sw a5, cpu_regs_t_a5(t1)
	sw a6, cpu_regs_t_a6(t1)
	sw a7, cpu_regs_t_a7(t1)
	sw s2, cpu_regs_t_s2(t1)
	sw s3, cpu_regs_t_s3(t1)
	sw s4, cpu_regs_t_s4(t1)
	sw s5, cpu_regs_t_s5(t1)
	sw s6, cpu_regs_t_s6(t1)
	sw s7, cpu_regs_t_s7(t1)
	sw s8, cpu_regs_t_s8(t1)
	sw s9, cpu_regs_t_s9(t1)
	sw s10, cpu_regs_t_s10(t1)
	sw s11, cpu_regs_t_s11(t1)
	sw t4, cpu_regs_t_t4(t1)
	sw t5, cpu_regs_t_t5(t1)
	sw t6, cpu_regs_t_t6(t1)
	.endm



	# Restore all regs not restored by `isr_exit`.
	# Assumes `kernel_ctx_t *` in t0 and `cpu_regs_t *` in t1.
	.macro restore_regs
	lw s0, cpu_regs_t_s0(t1)
	lw s1, cpu_regs_t_s1(t1)
	lw a0, cpu_regs_t_a0(t1)
	lw a1, cpu_regs_t_a1(t1)
	lw a2, cpu_regs_t_a2(t1)
	lw a3, cpu_regs_t_a3(t1)
	lw a4, cpu_regs_t_a4(t1)
	lw a5, cpu_regs_t_a5(t1)
	lw a6, cpu_regs_t_a6(t1)
	lw a7, cpu_regs_t_a7(t1)
	lw s2, cpu_regs_t_s2(t1)
	lw s3, cpu_regs_t_s3(t1)
	lw s4, cpu_regs_t_s4(t1)
	lw s5, cpu_regs_t_s5(t1)
	lw s6, cpu_regs_t_s6(t1)
	lw s7, cpu_regs_t_s7(t1)
	lw s8, cpu_regs_t_s8(t1)
	lw s9, cpu_regs_t_s9(t1)
	lw s10, cpu_regs_t_s10(t1)
	lw s11, cpu_regs_t_s11(t1)
	lw t4, cpu_regs_t_t4(t1)
	lw t5, cpu_regs_t_t5(t1)
	lw t6, cpu_regs_t_t6(t1)
	.endm





	# Trap and system call handler.
	.text
	.type __trap_asm, %function
	.align 2
__trap_asm:
	isr_entry
	# Separate system calls from other traps.
	csrr t2, mcause
	li t3, 8
	bltu t2, t3, .trap_save_regs
	li t3, 11
	bleu t2, t3, .syscall
	
.trap_save_regs:
	# This is a non-syscall trap; saving all registers is mandatory.
	save_regs
	# Most of the trap handler is implemented in C.
	jal __trap_handler
	get_kctx
	
	# Check for outstanding context switch.
	# If nonnull, context will be switched.
	lw t2, kernel_ctx_t_ctxswitch(t0)
	beq t2, x0, .trap_restore_regs
	# All that is necessary is swapping out t0.
	mv t0, t2
	
.trap_restore_regs:
	# This is a non-syscall trap; restoring all registers is mandatory.
	restore_regs
	j .trap_exit
	
.syscall:
	# This is a system call; only callee-saved registers are saved.
	# Increment the program counter past the initiating `ecall` instruction.
	lw t2, cpu_regs_t_pc(t1)
	addi t2, t2, 4
	sw t2, cpu_regs_t_pc(t1)
	# Most of the syscall handler is implemented in C.
	jal __syscall_handler
	get_kctx
	
	# Check for outstanding context switch.
	# If nonnull, context will be switched.
	lw t2, kernel_ctx_t_ctxswitch(t0)
	beq t2, x0, .trap_exit
	# General registers were not saved and must be done explicitly.
	save_regs
	mv t0, t2
	restore_regs
	
.trap_exit:
	isr_exit
	mret





	# Interrupt handler.
	.text
	.type __isr_asm, %function
	.align 2
__isr_asm:
	# This is an interrupt; saving all registers is mandatory.
	isr_entry
	save_regs
	# Most of the interrupt handler is implemented in C.
	jal __interrupt_handler
	get_kctx
	
	# Check for outstanding context switch.
	# If nonnull, context will be switched.
	lw t2, kernel_ctx_t_ctxswitch(t0)
	beq t2, x0, .isr_exit
	# All that is necessary is swapping out t0.
	mv t0, t2
	
.isr_exit:
	# This is an interrupt; restoring all registers is mandatory.
	restore_regs
	isr_exit
	mret





	# Interrupt and trap handler stack.
	.section ".bss"
__interrupt_stack_lo:
	.skip ISR_STACK_DEPTH*4
__interrupt_stack_hi:





	# Interrupt vector table for the CPU.
	# This must be aligned to a 256-byte boundary, so it is in a special section.
	.section ".interrupt_vector_table"
__interrupt_vector_table:
	.option push
	.option norvc
	j __trap_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	j __isr_asm
	.option pop
	