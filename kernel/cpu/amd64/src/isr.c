
// SPDX-License-Identifier: MIT

#include "cpu/isr.h"

#include "backtrace.h"
#include "cpu/isr_ctx.h"
#include "interrupt.h"
#include "log.h"
#include "panic.h"
#include "port/hardware.h"
#include "process/internal.h"
#include "process/sighandler.h"
#include "process/types.h"
#include "rawprint.h"
#include "scheduler/cpu.h"
#include "scheduler/types.h"
#if MEMMAP_VMEM
#include "cpu/mmu.h"
#include "memprotect.h"
#endif



// Kill a process from a trap / ISR.
static void kill_proc_on_trap() {
    proc_exit_self(-1);
    irq_disable();
    sched_lower_from_isr();
    isr_context_switch();
    __builtin_unreachable();
}

// Called from ASM on non-system call trap.
void amd64_trap_handler() {
    // TODO.
}

// Return a value from the syscall handler.
void syscall_return(long long value) {
    sched_thread_t *thread = isr_ctx_get()->thread;
    isr_ctx_t      *usr    = &thread->user_isr_ctx;

    usr->regs.rax = value;
    usr->regs.rip = usr->regs.rcx;

    if (proc_signals_pending_raw(thread->process)) {
        proc_signal_handler();
    }
    irq_disable();
    sched_lower_from_isr();
    isr_context_switch();
    __builtin_unreachable();
}
