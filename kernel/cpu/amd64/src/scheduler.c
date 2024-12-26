
// SPDX-License-Identifier: MIT

#include "scheduler/scheduler.h"

#include "assertions.h"
#include "backtrace.h"
#include "badge_strings.h"
#include "isr_ctx.h"
#include "log.h"
#include "process/internal.h"
#include "process/process.h"
#include "process/types.h"
#include "scheduler/cpu.h"
#include "scheduler/isr.h"
#if MEMMAP_VMEM
#include "cpu/mmu.h"
#endif



// Requests the scheduler to prepare a switch from userland to kernel for a user thread.
// If `syscall` is true, copies registers `a0` through `a7` to the kernel thread.
// Sets the program counter for the thread to `pc`.
void sched_raise_from_isr(sched_thread_t *thread, bool syscall, void *entry_point) {
    assert_dev_drop(!(thread->flags & THREAD_KERNEL) && !(thread->flags & THREAD_PRIVILEGED));
    thread->flags |= THREAD_PRIVILEGED;

    // TODO: Shuffle data between registers.

    // Do time accounting.
    timestamp_us_t    now         = time_us();
    sched_cpulocal_t *info        = isr_ctx_get()->cpulocal->sched;
    timestamp_us_t    used        = now - info->last_preempt;
    thread->timeusage.cycle_time += used;
    thread->timeusage.user_time  += used;
    info->last_preempt            = now;

    // Set context switch target to kernel thread.
    isr_ctx_switch_set(&thread->kernel_isr_ctx);
}

// Requests the scheduler to prepare a switch from kernel to userland for a user thread.
// Resumes the userland thread where it left off.
void sched_lower_from_isr() {
    sched_thread_t *thread  = sched_current_thread();
    process_t      *process = thread->process;
    assert_dev_drop(!(thread->flags & THREAD_KERNEL) && (thread->flags & THREAD_PRIVILEGED));
    atomic_fetch_and(&thread->flags, ~THREAD_PRIVILEGED);

    // Do time accounting.
    timestamp_us_t    now          = time_us();
    sched_cpulocal_t *info         = isr_ctx_get()->cpulocal->sched;
    timestamp_us_t    used         = now - info->last_preempt;
    thread->timeusage.cycle_time  += used;
    thread->timeusage.kernel_time += used;
    info->last_preempt             = now;

    // Set context switch target to user thread.
    isr_ctx_switch_set(&thread->user_isr_ctx);
    assert_dev_drop(!(thread->user_isr_ctx.flags & ISR_CTX_FLAG_KERNEL));

    if (atomic_load(&process->flags) & PROC_EXITING) {
        // Request a context switch to a different thread.
        sched_request_switch_from_isr();
    }
}

// Check whether the current thread is in a signal handler.
// Returns signal number, or 0 if not in a signal handler.
bool sched_is_sighandler() {
    sched_thread_t *thread = sched_current_thread();
    return atomic_load(&thread->flags) & THREAD_SIGHANDLER;
}

// Enters a signal handler in the current thread.
// Returns false if there isn't enough resources to do so.
bool sched_signal_enter(size_t handler_vaddr, size_t return_vaddr, int signum) {
    sched_thread_t *thread = sched_current_thread();

    // Ensure the user has enough stack.
    size_t usp   = thread->user_isr_ctx.regs.rsp;
    size_t usize = sizeof(size_t) * 20;
    if ((proc_map_contains_raw(thread->process, usp - usize, usize) & MEMPROTECT_FLAG_RW) != MEMPROTECT_FLAG_RW) {
        // Not enough stack that the process owns.
        return false;
    }
    thread->user_isr_ctx.regs.rsp -= usize;

    // Save context to user's stack.
#if MEMMAP_VMEM
    mmu_enable_sum();
#endif
    size_t *stackptr = (size_t *)thread->user_isr_ctx.regs.rsp;
    // TODO: Save caller-save registers.
#if MEMMAP_VMEM
    mmu_disable_sum();
#endif

    // Set up registers for entering signal handler.
    // TODO.

    // Successfully entered signal handler.
    atomic_fetch_or(&thread->flags, THREAD_SIGHANDLER);
    return true;
}

// Exits a signal handler in the current thread.
// Returns false if the process cannot be resumed.
bool sched_signal_exit() {
    sched_thread_t *thread = sched_current_thread();
    if (!(atomic_fetch_and(&thread->flags, ~THREAD_SIGHANDLER) & THREAD_SIGHANDLER)) {
        return false;
    }

    // Ensure the user still has the stack.
    size_t usp   = thread->user_isr_ctx.regs.rsp;
    size_t usize = sizeof(size_t) * 20;
    if ((proc_map_contains_raw(thread->process, usp, usize) & MEMPROTECT_FLAG_RW) != MEMPROTECT_FLAG_RW) {
        // If this happens, the process probably corrupted it's own stack.
        return false;
    }

// Restore user's state.
#if MEMMAP_VMEM
    mmu_enable_sum();
#endif
    size_t *stackptr = (size_t *)thread->user_isr_ctx.regs.rsp;
    // TODO: Restore user's caller-save registers.
#if MEMMAP_VMEM
    mmu_disable_sum();
#endif

    // Restore user's stack pointer.
    // TODO.

    // Successfully returned from signal handler.
    return true;
}

// Return to exit the thread.
static void sched_exit_self(int code) {
#ifndef NDEBUG
    sched_thread_t *const thread = sched_current_thread();
    logkf(LOG_DEBUG, "Kernel thread '%{cs}' returned %{d}", thread->name, code);
#endif
    thread_exit(code);
}

// Prepares a context to be invoked as a kernel thread.
void sched_prepare_kernel_entry(sched_thread_t *thread, void *entry_point, void *arg) {
    // Initialize registers.
    mem_set(&thread->kernel_isr_ctx.regs, 0, sizeof(thread->kernel_isr_ctx.regs));
    // TODO.
}

// Prepares a pair of contexts to be invoked as a userland thread.
// Kernel-side in these threads is always started by an ISR and the entry point is given at that time.
void sched_prepare_user_entry(sched_thread_t *thread, size_t entry_point, size_t arg) {
    // Initialize kernel registers.
    mem_set(&thread->kernel_isr_ctx.regs, 0, sizeof(thread->kernel_isr_ctx.regs));
    // TODO.
}
