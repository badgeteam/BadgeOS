
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
#define SCHED_MIN_US        5000
// The time quota increment per increased priority.
#define SCHED_INC_US        500
// The microsecond interval on which schedulers measure CPU load.
#define SCHED_LOAD_INTERVAL 250000



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
// The thread should be suspended even if it is a kernel thread.
#define THREAD_KSUSPEND   (1 << 8)
// The thread has exited and is awaiting join.
#define THREAD_EXITED     (1 << 9)
// The thread is blocked on a resource.
#define THREAD_BLOCKED    (1 << 10)

// The scheduler is starting on this CPU.
#define SCHED_STARTING (1 << 0)
// The scheduler is running on this CPU.
#define SCHED_RUNNING  (1 << 1)
// The scheduler is pending exit on this CPU.
#define SCHED_EXITING  (1 << 2)

// Things a thread can be blocked on.
typedef enum {
    // Thread is blocked on a `mutex_t`.
    THREAD_BLOCK_MUTEX,
} thread_block_t;

// Thread struct.
struct sched_thread_t {
    // Thread queue link.
    dlist_node_t node;

    // Process to which this thread belongs.
    process_t  *process;
    // Lowest address of the kernel stack.
    size_t      kernel_stack_bottom;
    // Highest address of the kernel stack.
    size_t      kernel_stack_top;
    // Priority of this thread.
    int         priority;
    // Time usage information.
    timeusage_t timeusage;

    // Thread flags.
    atomic_int     flags;
    // Exit code from `thread_exit`
    int            exit_code;
    // Cause for the thread to block. Only valid if THREAD_BLOCKED flag is set.
    thread_block_t blocked_by;
    // Information for the object the thread is blocking on.
    union {
        // Info for threads blocked on a mutex.
        struct {
            // Pointer to blocking mutex.
            mutex_t *mutex;
            // Timer ID used by mutex timeout code.
            int64_t  timer_id;
        } mutex;
    } blocking_obj;

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
    // Last preemption time.
    timestamp_us_t last_preempt;
    // Time until next measurement interval.
    timestamp_us_t load_measure_time;
    // CPU load average in 0.01% increments.
    atomic_int     load_average;
    // CPU load estimate in 0.01% increments.
    atomic_int     load_estimate;
    // Idle thread.
    sched_thread_t idle_thread;
};
