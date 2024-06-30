
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/regs.h"
#include "cpu/riscv.h"
#include "isr_ctx.h"

#ifndef __ASSEMBLER__
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

// Word size of the stack used for interrupt and trap handlers.
#define ISR_STACK_DEPTH 4096

#ifdef __ASSEMBLER__
// clang-format off

// Interrupt vector table implemented in ASM.
    .global riscv_interrupt_vector_table
// Called from ASM on interrupt.
    .global riscv_interrupt_handler
// Called from ASM on trap.
    .global riscv_trap_handler

// clang-format on
#else
// Interrupt vector table implemented in ASM.
extern uint32_t riscv_interrupt_vector_table[32];
// Callback from ASM to platform-specific interrupt handler.
extern void     riscv_interrupt_handler();
// ASM system call wrapper function.
extern void     riscv_syscall_wrapper();
// Callback from ASM on non-syscall trap.
extern void     riscv_trap_handler();
// Return a value from the syscall handler.
extern void     syscall_return(long long value) __attribute__((noreturn));

// Disable interrupts and return whether they were enabled.
static inline bool isr_global_disable() {
    uint32_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));
    asm volatile("csrc mstatus, %0" ::"r"((1U << RISCV_STATUS_MIE_BIT)));
    return mstatus & (1U << RISCV_STATUS_MIE_BIT);
}
// Enable interrupts.
static inline void isr_global_enable() {
    asm volatile("csrs mstatus, %0" ::"r"((1U << RISCV_STATUS_MIE_BIT)));
}
// Explicit context switch from M-mode.
// Interrupts must be disabled on entry and will be re-enabled on exit.
// If the context switch target is not set, this is a NOP.
extern void isr_context_switch();

#endif
