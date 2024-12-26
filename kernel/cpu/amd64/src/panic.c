
// SPDX-License-Identifier: MIT

#include "panic.h"

#include "backtrace.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "log.h"
#include "rawprint.h"

void abort() {
    panic_abort();
}

// Call this function when and only when the kernel has encountered a fatal error.
// Prints register dump for current kernel context and jumps to `panic_poweroff`.
void panic_abort() {
    irq_disable();
    logkf_from_isr(LOG_FATAL, "`panic_abort()` called!");
    backtrace();
    kernel_cur_regs_dump();
    panic_poweroff();
}

// Call this function when and only when the kernel has encountered a fatal error.
// Immediately power off or reset the system.
void panic_poweroff() {
    rawprint("**** KERNEL PANIC ****\nhalted\n");
    // TODO: Disable interrupts.
    while (1) asm volatile("hlt");
}
