
// SPDX-License-Identifier: MIT

#include "scheduler/scheduler.h"

#include "assertions.h"
#include "badge_strings.h"
#include "isr_ctx.h"
#include "log.h"
#include "process/internal.h"
#include "process/process.h"
#include "process/types.h"
#include "scheduler/cpu.h"
#include "scheduler/isr.h"



// Requests the scheduler to prepare a switch from userland to kernel for a user thread.
// If `syscall` is true, copies registers `a0` through `a7` to the kernel thread.
// Sets the program counter for the thread to `pc`.
void sched_raise_from_isr(sched_thread_t *thread, bool syscall, void *entry_point) {
    assert_dev_drop(!(thread->flags & THREAD_KERNEL) && !(thread->flags & THREAD_PRIVILEGED));
    thread->flags |= THREAD_PRIVILEGED;

    // Set kernel thread entrypoint.
    thread->kernel_isr_ctx.regs.pc = (uint32_t)entry_point;
    thread->kernel_isr_ctx.regs.sp = thread->kernel_stack_top;
    thread->kernel_isr_ctx.regs.s0 = 0;
    thread->kernel_isr_ctx.regs.ra = 0;

    if (syscall) {
        // Copy syscall arg registers.
        thread->kernel_isr_ctx.regs.a0 = thread->user_isr_ctx.regs.a0;
        thread->kernel_isr_ctx.regs.a1 = thread->user_isr_ctx.regs.a1;
        thread->kernel_isr_ctx.regs.a2 = thread->user_isr_ctx.regs.a2;
        thread->kernel_isr_ctx.regs.a3 = thread->user_isr_ctx.regs.a3;
        thread->kernel_isr_ctx.regs.a4 = thread->user_isr_ctx.regs.a4;
        thread->kernel_isr_ctx.regs.a5 = thread->user_isr_ctx.regs.a5;
        thread->kernel_isr_ctx.regs.a6 = thread->user_isr_ctx.regs.a6;
        thread->kernel_isr_ctx.regs.a7 = thread->user_isr_ctx.regs.a7;
    }

    // Set context switch target to kernel thread.
    isr_ctx_switch_set(&thread->kernel_isr_ctx);
}

// Requests the scheduler to prepare a switch from kernel to userland for a user thread.
// Resumes the userland thread where it left off.
void sched_lower_from_isr() {
    sched_thread_t *thread  = sched_get_current_thread_unsafe();
    process_t      *process = thread->process;
    assert_dev_drop(!(thread->flags & THREAD_KERNEL) && (thread->flags & THREAD_PRIVILEGED));
    thread->flags &= ~THREAD_PRIVILEGED;

    // Set context switch target to user thread.
    isr_ctx_switch_set(&thread->user_isr_ctx);
    assert_dev_drop(!thread->user_isr_ctx.is_kernel_thread);

    if (atomic_load(&process->flags) & PROC_EXITING) {
        // Request a context switch to a different thread.
        thread->flags &= ~THREAD_RUNNING;
        sched_request_switch_from_isr();
    }
}

// Check whether the current thread is in a signal handler.
// Returns signal number, or 0 if not in a signal handler.
bool sched_is_sighandler() {
    sched_thread_t *thread = sched_get_current_thread();
    return thread->flags & THREAD_SIGHANDLER;
}

// Enters a signal handler in the current thread.
// Returns false if there isn't enough resources to do so.
bool sched_signal_enter(size_t handler_vaddr, size_t return_vaddr, int signum) {
    sched_thread_t *thread = sched_get_current_thread();

    // Ensure the user has enough stack.
    size_t usp   = thread->user_isr_ctx.regs.sp;
    size_t usize = sizeof(uint32_t) * 20;
    if ((proc_map_contains_raw(thread->process, usp - usize, usize) & MEMPROTECT_FLAG_RW) != MEMPROTECT_FLAG_RW) {
        // Not enough stack that the process owns.
        return false;
    }
    thread->user_isr_ctx.regs.sp -= usize;

    // Save context to user's stack.
    // TODO: Enable SUM bit for S-mode kernel.
    uint32_t *stackptr = (uint32_t *)thread->user_isr_ctx.regs.sp;
    stackptr[0]        = thread->user_isr_ctx.regs.t0;
    stackptr[1]        = thread->user_isr_ctx.regs.t1;
    stackptr[2]        = thread->user_isr_ctx.regs.t2;
    stackptr[3]        = thread->user_isr_ctx.regs.a0;
    stackptr[4]        = thread->user_isr_ctx.regs.a1;
    stackptr[5]        = thread->user_isr_ctx.regs.a2;
    stackptr[6]        = thread->user_isr_ctx.regs.a3;
    stackptr[7]        = thread->user_isr_ctx.regs.a4;
    stackptr[8]        = thread->user_isr_ctx.regs.a5;
    stackptr[9]        = thread->user_isr_ctx.regs.a6;
    stackptr[10]       = thread->user_isr_ctx.regs.a7;
    stackptr[11]       = thread->user_isr_ctx.regs.t3;
    stackptr[12]       = thread->user_isr_ctx.regs.t4;
    stackptr[13]       = thread->user_isr_ctx.regs.t5;
    stackptr[14]       = thread->user_isr_ctx.regs.t6;
    stackptr[17]       = thread->user_isr_ctx.regs.pc;
    stackptr[18]       = thread->user_isr_ctx.regs.s0;
    stackptr[19]       = thread->user_isr_ctx.regs.ra;
    // TODO: Disable SUM bit for S-mode kernel.

    // Set up registers for entering signal handler.
    thread->user_isr_ctx.regs.s0 = thread->user_isr_ctx.regs.sp + usize;
    thread->user_isr_ctx.regs.ra = return_vaddr;
    thread->user_isr_ctx.regs.pc = handler_vaddr;
    thread->user_isr_ctx.regs.a0 = signum;

    // Successfully entered signal handler.
    thread->flags |= THREAD_SIGHANDLER;
    return true;
}

// Exits a signal handler in the current thread.
// Returns false if the process cannot be resumed.
bool sched_signal_exit() {
    sched_thread_t *thread  = sched_get_current_thread_unsafe();
    thread->flags          &= ~THREAD_SIGHANDLER;

    // Ensure the user still has the stack.
    size_t usp   = thread->user_isr_ctx.regs.sp;
    size_t usize = sizeof(uint32_t) * 20;
    if ((proc_map_contains_raw(thread->process, usp, usize) & MEMPROTECT_FLAG_RW) != MEMPROTECT_FLAG_RW) {
        // If this happens, the process probably corrupted it's own stack.
        return false;
    }

    // Restore user's state.
    // TODO: Enable SUM bit for S-mode kernel.
    uint32_t *stackptr           = (uint32_t *)thread->user_isr_ctx.regs.sp;
    thread->user_isr_ctx.regs.t0 = stackptr[0];
    thread->user_isr_ctx.regs.t1 = stackptr[1];
    thread->user_isr_ctx.regs.t2 = stackptr[2];
    thread->user_isr_ctx.regs.a0 = stackptr[3];
    thread->user_isr_ctx.regs.a1 = stackptr[4];
    thread->user_isr_ctx.regs.a2 = stackptr[5];
    thread->user_isr_ctx.regs.a3 = stackptr[6];
    thread->user_isr_ctx.regs.a4 = stackptr[7];
    thread->user_isr_ctx.regs.a5 = stackptr[8];
    thread->user_isr_ctx.regs.a6 = stackptr[9];
    thread->user_isr_ctx.regs.a7 = stackptr[10];
    thread->user_isr_ctx.regs.t3 = stackptr[11];
    thread->user_isr_ctx.regs.t4 = stackptr[12];
    thread->user_isr_ctx.regs.t5 = stackptr[13];
    thread->user_isr_ctx.regs.t6 = stackptr[14];
    thread->user_isr_ctx.regs.pc = stackptr[17];
    thread->user_isr_ctx.regs.s0 = stackptr[18];
    thread->user_isr_ctx.regs.ra = stackptr[19];
    // TODO: Disable SUM bit for S-mode kernel.

    // Restore user's stack pointer.
    thread->user_isr_ctx.regs.sp += usize;

    // Successfully returned from signal handler.
    return true;
}

// Return to exit the thread.
static void sched_exit_self() {
#ifndef NDEBUG
    sched_thread_t *const this_thread = sched_get_current_thread();
    logkf(LOG_INFO, "Kernel thread '%{cs}' returned", sched_get_name(this_thread));
#endif
    sched_exit(0);
}

// Prepares a context to be invoked as a kernel thread.
void sched_prepare_kernel_entry(sched_thread_t *thread, sched_entry_point_t entry_point, void *arg) {
    // Initialize registers.
    mem_set(&thread->kernel_isr_ctx.regs, 0, sizeof(thread->kernel_isr_ctx.regs));
    thread->kernel_isr_ctx.regs.pc = (uint32_t)entry_point;
    thread->kernel_isr_ctx.regs.sp = thread->kernel_stack_top;
    thread->kernel_isr_ctx.regs.a0 = (uint32_t)arg;
    thread->kernel_isr_ctx.regs.ra = (uint32_t)sched_exit_self;
    asm("sw gp, %0" ::"m"(thread->kernel_isr_ctx.regs.gp));
}

// Prepares a pair of contexts to be invoked as a userland thread.
// Kernel-side in these threads is always started by an ISR and the entry point is given at that time.
void sched_prepare_user_entry(sched_thread_t *thread, sched_entry_point_t entry_point, void *arg) {
    // Initialize kernel registers.
    mem_set(&thread->kernel_isr_ctx.regs, 0, sizeof(thread->kernel_isr_ctx.regs));
    thread->kernel_isr_ctx.regs.sp = thread->kernel_stack_top;
    asm("sw gp, %0" ::"m"(thread->kernel_isr_ctx.regs.gp));

    // Initialize userland registers.
    mem_set(&thread->user_isr_ctx.regs, 0, sizeof(thread->user_isr_ctx.regs));
    thread->user_isr_ctx.regs.pc = (uint32_t)entry_point;
    thread->user_isr_ctx.regs.a0 = (uint32_t)arg;
}
