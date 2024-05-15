
// SPDX-License-Identifier: MIT

#include "cpu/panic.h"

#include "interrupt.h"
#include "isr_ctx.h"
#include "log.h"
#include "rawprint.h"

// Call this function when and only when the kernel has encountered a fatal error.
// Prints register dump for current kernel context and jumps to `panic_poweroff`.
void panic_abort() {
    irq_enable(false);
    logkf_from_isr(LOG_FATAL, "`panic_abort()` called!");
    kernel_cur_regs_dump();
    panic_poweroff();
}

// Call this function when and only when the kernel has encountered a fatal error.
// Immediately power off or reset the system.
void panic_poweroff() {
    rawprint("**** KERNEL PANIC ****\nhalted\n");
    asm volatile("csrci mstatus, 0xa");
    while (1) asm volatile("wfi");
}
