
// SPDX-License-Identifier: MIT

#pragma once

#ifndef __ASSEMBLER__
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#endif
#include "cpu/regs.h"
#include "cpu/kernel_ctx.h"

// Word size of the stack used for interrupt and trap handlers.
#define ISR_STACK_DEPTH 4096

#ifdef __ASSEMBLER__
	# Interrupt vector table implemented in ASM.
	.global __interrupt_vector_table
	# Called from ASM on interrupt.
	.global __interrupt_handler
	# Called from ASM on system call.
	.global __syscall_handler
	# Called from ASM on non-syscall trap.
	.global __trap_handler
#else
// Interrupt vector table implemented in ASM.
extern void *__interrupt_vector_table[32];
// Callback from ASM to platform-specific interrupt handler.
extern void __interrupt_handler();
// Callback from ASM to syscall implementation.
#define __SYSCALL_HANDLER_ARGS long a0, long a1, long a2, long a3, long a4, long a5, long a6, long sysno
// Callback from ASM to syscall implementation.
extern void __syscall_handler(__SYSCALL_HANDLER_ARGS);
// Callback from ASM on non-syscall trap.
extern void __trap_handler();
#endif
