
// SPDX-License-Identifier: MIT

#include "cpu/isr.h"

#include "backtrace.h"
#include "cpu/isr_ctx.h"
#include "cpu/panic.h"
#include "interrupt.h"
#include "log.h"
#include "port/hardware.h"
#include "process/internal.h"
#include "process/sighandler.h"
#include "process/types.h"
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
    NULL, // "ECALL from U-mode",
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

// Kill a process from a trap / ISR.
static void kill_proc_on_trap() {
    proc_exit_self(-1);
    isr_global_disable();
    sched_lower_from_isr();
    isr_context_switch();
    __builtin_unreachable();
}

// Called from ASM on non-system call trap.
void riscv_trap_handler() {
    // TODO: Per-CPU double trap detection.
    static int trap_depth = 0;

    long mcause, mstatus, mtval, mepc;
    asm volatile("csrr %0, mcause" : "=r"(mcause));
    long trapno = mcause & RISCV_VT_ICAUSE_MASK;
    if (mcause < 0) {
        riscv_interrupt_handler();
        return;
    }
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));

    trap_depth++;
    isr_ctx_t recurse_ctx;
    recurse_ctx.mpu_ctx          = NULL;
    recurse_ctx.is_kernel_thread = true;
    recurse_ctx.use_sp           = true;
    isr_ctx_t *kctx              = isr_ctx_swap(&recurse_ctx);
    recurse_ctx.thread           = kctx->thread;

    if (!kctx->is_kernel_thread) {
        switch (trapno) {
            default: break;

            case RISCV_TRAP_U_ECALL:
                // ECALL from U-mode goes to system call handler.
                sched_raise_from_isr(kctx->thread, true, syscall_handler);
                isr_ctx_swap(kctx);
                trap_depth--;
                return;

            case RISCV_TRAP_IACCESS:
            case RISCV_TRAP_LACCESS:
            case RISCV_TRAP_SACCESS:
            case RISCV_TRAP_IPAGE:
            case RISCV_TRAP_LPAGE:
            case RISCV_TRAP_SPAGE:
                // Memory access faults go to the SIGSEGV handler.
                sched_raise_from_isr(kctx->thread, true, proc_sigsegv_handler);
                isr_ctx_swap(kctx);
                trap_depth--;
                return;

            case RISCV_TRAP_IILLEGAL:
                // Memory access faults go to the SIGILL handler.
                sched_raise_from_isr(kctx->thread, true, proc_sigill_handler);
                isr_ctx_swap(kctx);
                trap_depth--;
                return;

            case RISCV_TRAP_EBREAK:
                // Memory access faults go to the SIGTRAP handler.
                sched_raise_from_isr(kctx->thread, true, proc_sigtrap_handler);
                isr_ctx_swap(kctx);
                trap_depth--;
                return;
        }
    }

    // Unhandled trap.
    rawprint("\033[0m");
    if (trap_depth >= 3) {
        rawprint("**** TRIPLE FAULT ****\n");
        panic_poweroff();
    } else if (trap_depth == 2) {
        rawprint("**** DOUBLE FAULT ****\n");
    }

    // Print trap name.
    if (trapno < TRAPNAMES_LEN && trapnames[trapno]) {
        rawprint(trapnames[trapno]);
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
    if (mtval && ((1 << trapno) & MEM_ADDR_TRAPS)) {
        rawprint(" while accessing 0x");
        rawprinthex(mtval, 8);
    } else if (mtval && trapno == RISCV_TRAP_IILLEGAL) {
        rawprint(" while decoding 0x");
        rawprinthex(mtval, 8);
    }

    rawputc('\r');
    rawputc('\n');

    // Print privilige mode.
    if (trap_depth == 1) {
        if (mstatus & (3 << RISCV_STATUS_MPP_BASE_BIT)) {
            rawprint("Running in kernel mode");
            if (!kctx->is_kernel_thread) {
                rawprint(" (despite is_kernel_thread=0)");
            }
        } else {
            rawprint("Running in user mode");
            if (kctx->is_kernel_thread) {
                rawprint(" (despite is_kernel_thread=1)");
            }
        }
    }

    // Print current process.
    if (trap_depth == 1 && kctx->thread && !(kctx->thread->flags & THREAD_KERNEL)) {
        rawprint(" in process ");
        rawprintdec(kctx->thread->process->pid, 1);
    }
    rawprint("\n");
    backtrace_from_ptr(kctx->frameptr);

    isr_ctx_dump(kctx);

    if (mstatus & (3 << RISCV_STATUS_MPP_BASE_BIT) || trap_depth > 1) {
        // When the kernel traps it's a bad time.
        panic_poweroff();
    } else {
        // When the user traps just stop the process.
        sched_raise_from_isr(kctx->thread, false, kill_proc_on_trap);
    }
    isr_ctx_swap(kctx);
    trap_depth--;
}

// Return a value from the syscall handler.
void syscall_return(long long value) {
    sched_thread_t *thread  = isr_ctx_get()->thread;
    isr_ctx_t      *usr     = &thread->user_isr_ctx;
    usr->regs.a0            = value;
    usr->regs.a1            = value >> 32;
    usr->regs.pc           += 4;
    if (proc_signals_pending_raw(thread->process)) {
        proc_signal_handler();
    }
    irq_enable(false);
    sched_lower_from_isr();
    isr_context_switch();
    __builtin_unreachable();
}
