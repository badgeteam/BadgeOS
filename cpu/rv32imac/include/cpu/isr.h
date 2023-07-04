
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/regs.h"
#include "cpu/riscv.h"
#include "kernel_ctx.h"

#ifndef __ASSEMBLER__
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

// Word size of the stack used for interrupt and trap handlers.
#define ISR_STACK_DEPTH 4096

#ifdef __ASSEMBLER__
#Interrupt vector table implemented in ASM.
.global                                __interrupt_vector_table
#Called from ASM on                    interrupt.
    .global                            __interrupt_handler
#Called from ASM on system             call.
    .global                            __syscall_handler
#Called from ASM on non - syscall trap.
    .global                       __trap_handler
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


// Disable interrupts and return whether they were enabled.
static inline bool isr_global_disable() {
    uint32_t mstatus;
    asm volatile("csrrc %0, mstatus, %1" : "=r"(mstatus) : "r"((1U << RV32_MSTATUS_MIE_BIT)));
    return mstatus & (1U << RV32_MSTATUS_MIE_BIT);
}
// Enable interrupts.
static inline void isr_global_enable() { asm volatile("csrs mstatus, %0" ::"r"((1U << RV32_MSTATUS_MIE_BIT))); }
// Invokes the interrupt routine `isr`. `isr` must be between 0 and 31.
extern void isr_invoke(uint32_t isr);

#endif
