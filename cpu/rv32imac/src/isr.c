
// SPDX-License-Identifier: MIT
// Note: This file implements placeholder ISRs and will be removed eventually.

#include "cpu/isr.h"

#include "cpu/kernel_ctx.h"
#include "cpu/panic.h"
#include "log.h"
#include "rawprint.h"



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

// Called from ASM on system call.
// TODO: Subject to be moved when systam calls are implemented.
void __syscall_handler(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long sysno) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)a5;
    (void)a6;
    (void)sysno;
    logk(LOG_DEBUG, "The system call!");
}

static bool double_trap = false;

// Called from ASM on non-system call trap.
// TODO: Subject to be moved to scheduler when scheduler is implemented.
void __trap_handler() {
    if (double_trap) {
        rawprint("Double trap!\n");
        panic_poweroff();
    }
    double_trap = true;

    uint32_t mcause, mtval, mepc;
    asm volatile("csrr %0, mcause" : "=r"(mcause));

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

    kernel_ctx_t *kctx;
    asm volatile("csrr %0, mscratch" : "=r"(kctx));
    kernel_ctx_dump(kctx);

    // When the kernel traps it's a bad time.
    panic_poweroff();
}
