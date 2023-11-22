#include "scheduler/scheduler.h"

#include "assertions.h"
#include "badge_strings.h"
#include "isr_ctx.h"
#include "log.h"
#include "process/process.h"
#include "process/types.h"
#include "scheduler/cpu.h"
#include "scheduler/isr.h"



// Requests the scheduler to prepare a switch from userland to kernel for a user thread.
// If `syscall` is true, copies registers `a0` through `a7` to the kernel thread.
// Sets the program counter for the thread to `pc`.
void sched_raise_from_isr(bool syscall, void *entry_point) {
    sched_thread_t *thread = sched_get_current_thread_unsafe();
    assert_dev_drop(!(thread->flags & THREAD_KERNEL) && !(thread->flags & THREAD_PRIVILEGED));
    thread->flags |= THREAD_PRIVILEGED;

    // Set kernel thread entrypoint.
    thread->kernel_isr_ctx.regs.pc = (uint32_t)entry_point;
    thread->kernel_isr_ctx.regs.sp = thread->kernel_stack_top;

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

    assert_always(mutex_acquire_shared(NULL, &process->mtx, PROC_MTX_TIMEOUT));
    if (process->flags & PROC_EXITING) {
        // Request a context switch to a different thread.
        thread->flags &= ~THREAD_RUNNING;
        sched_request_switch_from_isr();

    } else {
        // Set context switch target to user thread.
        isr_ctx_switch_set(&thread->user_isr_ctx);
    }
    mutex_release_shared(NULL, &process->mtx);
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
