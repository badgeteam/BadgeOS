
// SPDX-License-Identifier: MIT

#pragma once

#include "attributes.h"
#include "badge_err.h"
#include "isr_ctx.h"
#include "list.h"
#include "process/process.h"
#include "scheduler.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



// The minimum time a thread will run. `SCHED_PRIO_LOW` maps to this.
#define SCHEDULER_MIN_TASK_TIME_US   5000 // 5ms
// The time quota increment per increased priority.
#define SCHEDULER_TIME_QUOTA_INCR_US 500 // 0.5ms * priority



// The thread is currently in the scheduling queues.
#define THREAD_RUNNING    (1 << 0)
// The thread has finished and is waiting for destruction.
#define THREAD_EXITING    (1 << 1)
// The thread is detached or has been joined.
#define THREAD_DETACHED   (1 << 2)
// The thread is a kernel thread.
#define THREAD_KERNEL     (1 << 3)
// The thread is a kernel thread or a user thread running in kernel mode.
#define THREAD_PRIVILEGED (1 << 4)
// The user thread is running a signal handler.
#define THREAD_SIGHANDLER (1 << 5)
// The thread should be added to the front of the queue.
#define THREAD_STARTNOW   (1 << 6)
// The thread should be suspended.
#define THREAD_SUSPENDING (1 << 7)
// The thread has exited and is awaiting join.
#define THREAD_EXITED     (1 << 8)

// The scheduler is running on this CPU.
#define SCHED_RUNNING (1 << 0)
// The scheduler is pending exit on this CPU.
#define SCHED_EXITING (1 << 1)

// Thread struct.
struct sched_thread_t {
    // Thread queue link.
    dlist_node_t node;

    // Process to which this thread belongs.
    process_t *process;
    // Lowest address of the kernel stack.
    size_t     kernel_stack_bottom;
    // Highest address of the kernel stack.
    size_t     kernel_stack_top;
    // Priority of this thread.
    int        priority;

    // Thread flags.
    atomic_int flags;
    // Exit code from `thread_exit`
    int        exit_code;

    // ISR context for threads running in kernel mode.
    isr_ctx_t kernel_isr_ctx;
    // ISR context for userland thread running in user mode.
    isr_ctx_t user_isr_ctx;

    // Thread ID.
    tid_t id;
    // Thread name.
    char *name;
};

// CPU-local scheduler data.
struct sched_cpulocal_t {
    // Scheduler start/stop mutex.
    mutex_t        run_mtx;
    // Incoming threads list mutex.
    mutex_t        incoming_mtx;
    // Threads pending handover to this CPU.
    dlist_t        incoming;
    // CPU-local thread queue.
    dlist_t        queue;
    // CPU-local scheduler state flags.
    atomic_int     flags;
    // CPU load estimate in 0.01% increments.
    atomic_int     load;
    // Idle thread.
    sched_thread_t idle_thread;
};

// Returns the current thread without using a critical section.
sched_thread_t *sched_current_thread_unsafe();
