
// SPDX-License-Identifier: MIT

#pragma once

#include "attributes.h"
#include "badge_err.h"
#include "isr_ctx.h"
#include "list.h"
#include "scheduler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



enum {
    // The minimum time a thread will run. `SCHED_PRIO_LOW` maps to this.
    SCHEDULER_MIN_TASK_TIME_US   = 1000, // 1ms
    // The time quota increment per increased priority.
    SCHEDULER_TIME_QUOTA_INCR_US = 100, // 0.1ms * priority
    // Quota for the idle task. This can be pretty high as the idle task
    // will only run when nothing else runs.
    // 1 second is a good measure, idle task will always be interrupted by other
    // means.
    SCHEDULER_IDLE_TASK_QUOTA_US = 1000000,
    // Defines how many threads are available in the kernel.
    // TODO: Replace this constant with a dynamically configurable allocator!
    SCHEDULER_MAX_THREADS        = 16,
    // Debug: Maximum length of a thread's name.
    SCHED_THREAD_NAME_LEN        = 32,
};

enum {
    // The thread is currently in the scheduling queues
    THREAD_RUNNING    = (1 << 0),
    // The thread has finished and is waiting for destruction
    THREAD_COMPLETED  = (1 << 1),
    // The thread is detached and will self-destroy after exit
    THREAD_DETACHED   = (1 << 2),
    // The thread is a kernel thread.
    THREAD_KERNEL     = (1 << 3),
    // The thread is a kernel thread or a user thread running in kernel mode.
    THREAD_PRIVILEGED = (1 << 4),
};

struct sched_thread_t {
    // Process to which this thread belongs.
    process_t   *process;
    // Lowest address of the kernel stack.
    size_t       kernel_stack_bottom;
    // Highest address of the kernel stack.
    size_t       kernel_stack_top;
    // Priority of this thread.
    sched_prio_t priority;

    // dynamic info:
    uint32_t     flags;
    dlist_node_t schedule_node;
    uint32_t     exit_code;

    // ISR context for threads running in kernel mode.
    isr_ctx_t kernel_isr_ctx;
    // ISR context for userland thread running in user mode.
    isr_ctx_t user_isr_ctx;

#ifndef NDEBUG
    // Name for debug printing.
    char name[SCHED_THREAD_NAME_LEN];
#endif
};

// Returns the current thread without using a critical section.
sched_thread_t *sched_get_current_thread_unsafe();
