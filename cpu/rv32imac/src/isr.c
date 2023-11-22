
// SPDX-License-Identifier: MIT
// Note: This file implements placeholder ISRs and will be removed eventually.

#include "cpu/isr.h"

#include "cpu/isr_ctx.h"
#include "cpu/panic.h"
#include "log.h"
#include "rawprint.h"
#include "scheduler/cpu.h"
#include "scheduler/types.h"



// Table of trap names.
static char const *const trapnames[] = {
    "Instruction address misaligned",
    "Instruction access fault",
    "Illegal instruction",
    "Breakpoint",
    "Load address misaligned",
    "Load access fault",
    "Store address misaligned",
    "Store access fault",
    "ECALL from U-mode",
    "ECALL from S-mode",
    NULL, // Reserved
    "ECALL from M-mode",
    "Instruction page fault",
    "Load page fault",
    NULL, // Reserved
    "Store page fault",
};
enum { TRAPNAMES_LEN = sizeof(trapnames) / sizeof(trapnames[0]) };

// Bitmask of traps that have associated memory addresses.
#define MEM_ADDR_TRAPS 0x00050f0

// Called from ASM on non-system call trap.
// TODO: Subject to be moved to scheduler when scheduler is implemented.
void __trap_handler() {
    uint32_t mcause, mstatus, mtval, mepc;
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));
    asm volatile("csrr %0, mcause" : "=r"(mcause));

    if (mcause == RV_TRAP_U_ECALL) {
        // ECALL from U-mode goes to system call handler instead of trap handler.
        sched_raise_from_isr(true, __syscall_handler);
        return;
    }

    // Print trap name.
    if (mcause < TRAPNAMES_LEN && trapnames[mcause]) {
        rawprint(trapnames[mcause]);
    } else {
        rawprint("Trap 0x");
        rawprinthex(mcause, 8);
    }

    // Print PC.
    asm volatile("csrr %0, mepc" : "=r"(mepc));
    rawprint(" at PC 0x");
    rawprinthex(mepc, 8);

    // Print trap value.
    asm volatile("csrr %0, mtval" : "=r"(mtval));
    if (mtval && ((1 << mcause) & MEM_ADDR_TRAPS)) {
        rawprint(" while accessing 0x");
        rawprinthex(mtval, 8);
    }

    rawputc('\r');
    rawputc('\n');

    // Print privilige mode.
    if (mstatus & (3 << RV32_MSTATUS_MPP_BASE_BIT)) {
        rawprint("Running in kernel mode\n");
    } else {
        rawprint("Running in user mode\n");
    }

    isr_ctx_t *kctx;
    asm volatile("csrr %0, mscratch" : "=r"(kctx));
    isr_ctx_dump(kctx);

    // When the kernel traps it's a bad time.
    panic_poweroff();
}

// Return a value from the syscall handler.
void __syscall_return(long long value) {
    isr_global_disable();
    isr_ctx_t *usr  = &isr_ctx_get()->thread->user_isr_ctx;
    usr->regs.a0    = value;
    usr->regs.a1    = value >> 32;
    usr->regs.pc   += 4;
    sched_lower_from_isr();
    isr_context_switch();
    __builtin_unreachable();
}
