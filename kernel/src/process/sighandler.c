
// SPDX-License-Identifier: MIT

#include "process/sighandler.h"

#include "backtrace.h"
#include "cpu/isr.h"
#include "interrupt.h"
#include "malloc.h"
#include "process/internal.h"
#include "process/types.h"
#include "scheduler/cpu.h"
#include "sys/wait.h"

// Signal name table.
char const *signames[SIG_COUNT] = {
    [SIGHUP] = "SIGHUP",   [SIGINT] = "SIGINT",       [SIGQUIT] = "SIGQUIT", [SIGILL] = "SIGILL",
    [SIGTRAP] = "SIGTRAP", [SIGABRT] = "SIGABRT",     [SIGBUS] = "SIGBUS",   [SIGFPE] = "SIGFPE",
    [SIGKILL] = "SIGKILL", [SIGUSR1] = "SIGUSR1",     [SIGSEGV] = "SIGSEGV", [SIGUSR2] = "SIGUSR2",
    [SIGPIPE] = "SIGPIPE", [SIGALRM] = "SIGALRM",     [SIGTERM] = "SIGTERM", [SIGSTKFLT] = "SIGSTKFLT",
    [SIGCHLD] = "SIGCHLD", [SIGCONT] = "SIGCONT",     [SIGSTOP] = "SIGSTOP", [SIGTSTP] = "SIGTSTP",
    [SIGTTIN] = "SIGTTIN", [SIGTTOU] = "SIGTTOU",     [SIGURG] = "SIGURG",   [SIGXCPU] = "SIGXCPU",
    [SIGXFSZ] = "SIGXFSZ", [SIGVTALRM] = "SIGVTALRM", [SIGPROF] = "SIGPROF", [SIGWINCH] = "SIGWINCH",
    [SIGIO] = "SIGIO",     [SIGPWR] = "SIGPWR",       [SIGSYS] = "SIGSYS",
};

// Runs the appropriate handler for a signal.
static void run_sighandler(int signum) {
    sched_thread_t  *thread = sched_get_current_thread_unsafe();
    process_t *const proc   = thread->process;
    // Check for signal handler.
    if (proc->sighandlers[signum] == SIG_DFL) {
        if ((SIG_DFL_KILL_MASK >> signum) & 1) {
            // Process didn't catch a signal that kills it.
            mutex_acquire(NULL, &log_mtx, TIMESTAMP_US_MAX);
            logkf_from_isr(LOG_ERROR, "Process %{d} received %{cs}", proc->pid, signames[signum]);
            // Print backtrace of the calling thread.
            backtrace_from_ptr((void *)thread->user_isr_ctx.regs.s0);
            isr_ctx_dump(&thread->user_isr_ctx);
            mutex_release(NULL, &log_mtx);
            // Finally, kill the process.
            proc_exit_self(W_SIGNALLED(signum));
        }
    } else if (proc->sighandlers[signum]) {
        sched_signal_enter(proc->sighandlers[signum], proc->sighandlers[0], signum);
    }
}

// Kernel side of the signal handler.
// Called in the kernel side of a used thread when a signal might be queued.
void proc_signal_handler() {
    process_t *const proc = proc_current();
    mutex_acquire(NULL, &proc->mtx, TIMESTAMP_US_MAX);
    if (proc->sigpending.len) {
        // Pop the first pending signal and run its handler.
        sigpending_t *node = (sigpending_t *)dlist_pop_front(&proc->sigpending);
        mutex_release(NULL, &proc->mtx);
        run_sighandler(node->signum);
        free(node);
    } else {
        mutex_release(NULL, &proc->mtx);
    }
    irq_enable(false);
    sched_lower_from_isr();
    isr_context_switch();
    __builtin_unreachable();
}

// Raises a fault signal to the current thread.
// If the thread is already running a signal handler, the process is killed.
static void trap_signal_handler(int signum) NORETURN;
static void trap_signal_handler(int signum) {
    sched_thread_t  *thread  = sched_get_current_thread_unsafe();
    process_t *const proc    = thread->process;
    int              current = sched_is_sighandler();
    if (current) {
        // If the thread is still running a signal handler, terminate the process.
        mutex_acquire(NULL, &log_mtx, TIMESTAMP_US_MAX);
        if (current > 0 && current < SIG_COUNT && signames[current]) {
            logkf_from_isr(
                LOG_ERROR,
                "Process %{d} received %{cs} while handling %{cs}",
                proc->pid,
                signames[signum],
                signames[current]
            );
        } else {
            logkf_from_isr(
                LOG_ERROR,
                "Process %{d} received %{cs} while handling Signal #%{d}",
                proc->pid,
                signames[signum],
                current
            );
        }
        // Print backtrace of the calling thread.
        backtrace_from_ptr((void *)thread->user_isr_ctx.regs.s0);
        isr_ctx_dump(&thread->user_isr_ctx);
        mutex_release(NULL, &log_mtx);
        // Finally, kill the process.
        proc_exit_self(W_SIGNALLED(signum));

    } else {
        // If the thread isn't running a signal handler, run the appropriate one.
        run_sighandler(signum);
    }
    irq_enable(false);
    sched_lower_from_isr();
    isr_context_switch();
    __builtin_unreachable();
}

// Raises a segmentation fault to the current thread.
// Called in the kernel side of a used thread when hardware detects a segmentation fault.
void proc_sigsegv_handler() {
    trap_signal_handler(SIGSEGV);
}

// Raises an illegal instruction fault to the current thread.
// Called in the kernel side of a used thread when hardware detects an illegal instruction fault.
void proc_sigill_handler() {
    trap_signal_handler(SIGILL);
}

// Raises a trace/breakpoint trap to the current thread.
// Called in the kernel side of a used thread when hardware detects a trace/breakpoint trap.
void proc_sigtrap_handler() {
    trap_signal_handler(SIGTRAP);
}

// Raises an invalid system call to the current thread.
// Called in the kernel side of a used thread when a system call does not exist.
void proc_sigsys_handler() {
    trap_signal_handler(SIGSYS);
}
