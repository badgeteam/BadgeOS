
// SPDX-License-Identifier: MIT
// Note: This file implements placeholder ISRs and will be removed eventually.

#include "cpu/isr.h"

#include "log.h"
#include "rawprint.h"
#include "cpu/panic.h"
#include "cpu/kernel_ctx.h"



// Table of trap names.
static const char *trapnames[32] = {
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
	NULL,
	"ECALL from M-mode",
	"Instruction page fault",
	"Load page fault",
	NULL,
	"Store page fault",
};
static const size_t trapnames_len = sizeof(trapnames) / sizeof(const char *);

// Bitmask of traps that have associated memory addresses.
#define MEM_ADDR_TRAPS 0x00050f0

// Called from ASM on system call.
// TODO: Subject to be moved when systam calls are implemented.
void __syscall_handler(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long sysno) {
	logk(LOG_DEBUG, "The system call!");
}

// Called from ASM on non-system call trap.
// TODO: Subject to be moved to scheduler when scheduler is implemented.
void __trap_handler() {
	uint32_t mcause, mtval, mepc;
	asm volatile ("csrr %0, mcause" : "=r" (mcause));
	
	// Print trap name.
	if (mcause < trapnames_len && trapnames[mcause]) {
		rawprint(trapnames[mcause]);
	} else {
		rawprint("Trap 0x");
		rawprinthex(mcause, 8);
	}
	
	// Print PC.
	asm volatile ("csrr %0, mepc" : "=r" (mepc));
	rawprint(" at PC 0x");
	rawprinthex(mepc, 8);
	
	// Print trap value.
	asm volatile ("csrr %0, mtval"  : "=r" (mtval));
	if (mtval && ((1 << mcause) & MEM_ADDR_TRAPS)) {
		rawprint(" while accessing 0x");
		rawprinthex(mtval, 8);
	}
	
	rawputc('\r');
	rawputc('\n');
	
	kernel_ctx_t *kctx;
	asm volatile ("csrr %0, mscratch" : "=r" (kctx));
	kernel_ctx_dump(kctx);
	
	// When the kernel traps it's a bad time.
	panic_poweroff();
}