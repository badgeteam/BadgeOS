
// SPDX-License-Identifier: MIT

#pragma once

#include "attributes.h"
#include "badge_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Processs struct.
typedef struct process_t      process_t;
// Globally unique thread ID.
typedef int                   tid_t;
// Thread struct.
typedef struct sched_thread_t sched_thread_t;
// Kernel thread entrypoint.
typedef int (*sched_entry_t)(void *arg);
// CPU-local scheduler data.
typedef struct sched_cpulocal_t sched_cpulocal_t;

// will be scheduled with smaller time slices than normal
#define SCHED_PRIO_LOW    0
// default value
#define SCHED_PRIO_NORMAL 10
// will be scheduled with bigger time slices than normal
#define SCHED_PRIO_HIGH   20



// Global scheduler initialization.
void sched_init();
// Start executing the scheduler on this CPU.
void sched_exec() NORETURN;
// Exit the scheduler and subsequenty shut down the CPU.
void sched_exit(int cpu);

// Create a new suspended userland thread.
// If `kernel_stack_bottom` is NULL, the scheduler will allocate a stack.
tid_t thread_new_user(
    badge_err_t *ec, char const *name, process_t *process, size_t user_entrypoint, size_t user_arg, int priority
);
// Create new suspended kernel thread.
// If `stack_bottom` is NULL, the scheduler will allocate a stack.
tid_t thread_new_kernel(badge_err_t *ec, char const *name, sched_entry_t entry_point, void *arg, int priority);
// Do not wait for thread to be joined; clean up immediately.
void  thread_detach(badge_err_t *ec, tid_t thread);

// Pauses execution of the thread.
void thread_suspend(badge_err_t *ec, tid_t thread);
// Resumes a previously suspended thread or starts it.
void thread_resume(badge_err_t *ec, tid_t thread);
// Resumes a previously suspended thread or starts it.
// Immediately schedules the thread instead of putting it in the queue first.
void thread_resume_now(badge_err_t *ec, tid_t thread);
// Returns whether a thread is running; it is neither suspended nor has it exited.
bool thread_is_running(badge_err_t *ec, tid_t thread);

// Returns the current thread ID.
tid_t           sched_current_tid();
// Returns the current thread struct.
sched_thread_t *sched_current_thread();
// Returns the associated thread struct.
sched_thread_t *sched_get_thread(tid_t thread);

// Explicitly yield to the scheduler; the scheduler may run other threads without waiting for preemption.
// Use this function to reduce the CPU time used by a thread.
void sched_yield(void);
// Exits the current thread.
// If the thread is detached, resources will be cleaned up.
void thread_exit(int code) NORETURN;
// Wait for another thread to exit.
void thread_join(tid_t thread);
