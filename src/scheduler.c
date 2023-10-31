
// SPDX-License-Identifier: MIT

#include "scheduler.h"

#include "assertions.h"
#include "attributes.h"
#include "badge_strings.h"
#include "cpu/isr.h"
#include "kernel_ctx.h"
#include "list.h"
#include "meta.h"
#include "port/hardware_allocation.h"
#include "port/interrupt.h"
#include "syscall.h"
#include "time.h"

#include <stdint.h>

// scheduler config:
enum {
    // The minimum time a thread will run. `SCHED_PRIO_LOW` maps to this.
    SCHEDULER_MIN_TASK_TIME_US = 1000, // 1ms

    // The time quota increment per increased priority.
    SCHEDULER_TIME_QUOTA_INCR_US = 100, // 0.1ms * priority

    // Quota for the idle task. This can be pretty high as the idle task
    // will only run when nothing else runs.
    // 1 second is a good measure, idle task will always be interrupted by other
    // means.
    SCHEDULER_IDLE_TASK_QUOTA_US = 1000000,

    // Defines how many threads are available in the kernel.
    // TODO: Replace this constant with a dynamically configurable allocator!
    SCHEDULER_MAX_THREADS = 16,
};


/// Returns non-0 value if `V` is aligned to `A`.
#define is_aligned(V, A) (((V) & ((A)-1)) == 0)

#define is_flag_set(C, F) (((C) & (F)) != 0)
#define reset_flag(C, F)  ((C) &= ~(F))
#define set_flag(C, F)    ((C) |= (F))

enum {
    // The thread is currently in the scheduling queues
    THREAD_RUNNING = (1 << 0),

    // The thread has finished and is waiting for destruction
    THREAD_COMPLETED = (1 << 1),

    // The thread is detached and will self-destroy after exit
    THREAD_DETACHED = (1 << 2),

    // The thread is a kernel thread.
    THREAD_KERNEL = (1 << 3),
};

static_assert((STACK_ALIGNMENT & (STACK_ALIGNMENT - 1)) == 0, "STACK_ALIGNMENT must be a power of two!");

enum {
    SCHED_THREAD_NAME_LEN = 32,
};

typedef enum thread_insert_position_t {
    INSERT_THREAD_BACK,
    INSERT_THREAD_FRONT,
} thread_insert_position_t;


struct sched_thread_t {
    // fixed info:
    process_t              *process;
    uintptr_t               stack_bottom;
    uintptr_t               stack_top;
    sched_thread_priority_t priority;

    // dynamic info:
    uint32_t     flags;
    dlist_node_t schedule_node;
    uint32_t     exit_code;

    // runtime state:
    kernel_ctx_t kernel_ctx;

#ifndef NDEBUG
    // debug info:
    char name[SCHED_THREAD_NAME_LEN];
#endif
};

// List of currently queued threads. `head` will be queued next, `tail` will be
// queued last.
static dlist_t thread_wait_queue = DLIST_EMPTY;

enum {
    // Size of the
    IDLE_TASK_STACK_LEN = 128
};
static uint8_t idle_task_stack[IDLE_TASK_STACK_LEN] ALIGNED_TO(STACK_ALIGNMENT);

static_assert(
    is_aligned(IDLE_TASK_STACK_LEN, STACK_ALIGNMENT), "IDLE_TASK_STACK_LEN must be aligned to STACK_ALIGNMENT!"
);

// The scheduler must schedule something, and the idle task is what
// the scheduler will schedule when nothing can be scheduled.
static sched_thread_t idle_task = {
    .stack_bottom = (uintptr_t)&idle_task_stack,
    .stack_top    = (uintptr_t)&idle_task_stack + IDLE_TASK_STACK_LEN,

#ifndef NDEBUG
    .name = "idle",
#endif
};

// Variable for safety. Is set to `true` once `sched_exec()` was called.
static bool scheduler_enabled = false;

// We need to have a flag for the first task switch, so don't suspend a
// non-existing task.
static bool scheduler_bootstrapped = false;

// Stores the time when the next interrupt routine should come along.
// This is then incremented gradually to keep the system running at a steady
// pace.
static timestamp_us_t next_isr_invocation_time;


enum {
    // A sentinel value stored in thread->flags to mark the thread as
    // non-allocated.
    // This is just a safety measure!
    THREAD_ALLOCATOR_SENTINEL = 0xBADC0DEUL,
};

// Backing store for the thread allocator.
static sched_thread_t thread_alloc_pool_storage[SCHEDULER_MAX_THREADS];

// Linked list of all available, non-allocated threads.
static dlist_t thread_alloc_pool = DLIST_EMPTY;


// Sanity check for critical sections
static bool critical_section_active = false;

// Stores whether a critical section had interrupts enabled before or not.
static bool critical_section_had_interrupts = false;

// Enters a scheduler-local critical section that cannot be interrupted from the
// scheduler itself. Call `leave_critical_section` after the critical section
// has ended.
//
// During a critical section, no thread switches can occurr.
static void enter_critical_section(void) {
    assert_dev_drop(!critical_section_active);
    critical_section_had_interrupts = isr_global_disable();
    critical_section_active         = true;
}

static void leave_critical_section(void) {
    assert_dev_drop(critical_section_active);
    if (critical_section_had_interrupts) {
        isr_global_enable();
    }
    critical_section_active = false;
}


// Allocates a new thread. Release memory with `thread_free` again.
static sched_thread_t *thread_alloc(void) {

    dlist_node_t *const node = dlist_pop_front(&thread_alloc_pool);
    if (node == NULL) {
        // out of memory
        return NULL;
    }

    sched_thread_t *const thread = field_parent_ptr(sched_thread_t, schedule_node, node);

    assert_dev_drop(thread->flags == THREAD_ALLOCATOR_SENTINEL);

#ifndef NDEBUG
    // for debug sessions, fill the thread with bogus data, so we can be sure
    // we initialized everything properly:
    mem_set(thread, 0xAA, sizeof(sched_thread_t));
#endif

    return thread;
}

// Frees a thread allocated with `thread_alloc`.
static void thread_free(sched_thread_t *thread) {

#ifndef NDEBUG
    // Set the thread memory to bogus data to make sure use-after-free is
    // more likely to be detected
    mem_set(thread, 0xAA, sizeof(sched_thread_t));
#endif

    // Store our sentinel and reset the schedule_node:
    thread->flags         = THREAD_ALLOCATOR_SENTINEL;
    thread->schedule_node = DLIST_NODE_EMPTY;

    // Push the thread back into the pool so `thread_alloc` can find it again
    dlist_append(&thread_alloc_pool, &thread->schedule_node);
}

static void idle_thread_function(void *arg) {
    (void)arg;
    while (true) {
        // make the loop not be undefined behaviour
        asm volatile("" ::: "memory");

        // TODO: Enable CPU sleeping here to save unnecessary power usage
    }
}

// Returns the current thread without using a critical section.
static sched_thread_t *sched_get_current_thread_unsafe(void) {
    if (!scheduler_enabled) {
        return NULL;
    }

    kernel_ctx_t *const kernel_ctx = kernel_ctx_get();
    return field_parent_ptr(sched_thread_t, kernel_ctx, kernel_ctx);
}

// Destroys a thread and releases its resources.
static void destroy_thread(sched_thread_t *thread) {
    assert_dev_drop(thread != NULL);

    if (is_flag_set(thread->flags, THREAD_RUNNING)) {
        // thread is still running, we have to remove it from the thread queue:
        sched_suspend_thread(NULL, thread);
    }

    // At last, we free the memory:
    thread_free(thread);
}

sched_thread_t *sched_get_current_thread(void) {
    enter_critical_section();
    sched_thread_t *const thread = sched_get_current_thread_unsafe();
    leave_critical_section();
    return thread;
}

#include "rawprint.h"

void sched_init(badge_err_t *const ec) {
    // Set up the idle task:
    sched_prepare_kernel_entry(&idle_task.kernel_ctx, idle_task.stack_top, idle_thread_function, NULL);

    // Initialize thread allocator:
    for (size_t i = 0; i < SCHEDULER_MAX_THREADS; i++) {
        thread_alloc_pool_storage[i] = (sched_thread_t){
            .flags         = THREAD_ALLOCATOR_SENTINEL,
            .schedule_node = DLIST_NODE_EMPTY,
        };
        dlist_append(&thread_alloc_pool, &thread_alloc_pool_storage[i].schedule_node);
    }

    badge_err_set_ok(ec);
}

void sched_exec(void) {
    scheduler_enabled = true;

    // as the "isr" is *now* invoked, we need the correct timestamp for
    // invocation to be set up correctly.
    next_isr_invocation_time = time_us();

    isr_global_disable();
    sched_request_switch_from_isr();
    isr_context_switch();

    // we can never reach this line, as the ISR will switch into the idle task
    __builtin_unreachable();
}

#include "log.h"

void sched_request_switch_from_isr(void) {
    if (!scheduler_enabled) {
        // only switch tasks when the scheduler is ready to run
        return;
    }

    // logk(LOG_INFO, "sched_request_switch_from_isr");

    timestamp_us_t const now = time_us();

    int64_t const time_quota_left = next_isr_invocation_time - now;


    if (scheduler_bootstrapped) {
        sched_thread_t *const current_thread = sched_get_current_thread_unsafe();
        // logkf(
        //     LOG_DEBUG,
        //     "yielding from task '%{cs}', flags=%{u32;x}",
        //     sched_get_name(current_thread),
        //     current_thread->flags
        // );
        if (current_thread == &idle_task) {
            // logk(LOG_DEBUG, "thread is idle task, do nothing special");
            // Idle task cannot be destroyed, idle task cannot be killed.

            if (time_quota_left > 0) {
                // TODO: Implement CPU usage statistics for threads
            }

        } else if (current_thread != NULL) {
            if (time_quota_left > 0) {
                // Thread is a good boy and didn't use up all of its time in the
                // CPU. We should give it some credit here.

                // TODO: Implement CPU usage statistics for threads
            }

            if (is_flag_set(current_thread->flags, THREAD_RUNNING)) {
                // logk(LOG_DEBUG, "thread is still running, put into queue again");

                // if we have a current thread, append it to the wait queue again
                // before popping the next task. This is necessary as we if we only
                // have a single task, that should be scheduled again. Otheriwse,
                // `dlist_pop_front` would return `NULL` instead of
                // `current_thread`.
                dlist_append(&thread_wait_queue, &current_thread->schedule_node);
            } else {
                // current thread is dead, we don't push it into the scheduler again

                if (is_flag_set(current_thread->flags, THREAD_COMPLETED)) {
                    if (is_flag_set(current_thread->flags, THREAD_DETACHED)) {
                        // logk(LOG_DEBUG, "thread is finished+detached, kill");
                        destroy_thread(current_thread);
                    } else {
                        // logk(LOG_DEBUG, "thread is finished, just stop");
                    }
                } else {
                    // logk(LOG_DEBUG, "thread is suspended");
                }
            }
        }
    } else {
        // First run must never call `sched_get_current_thread_unsafe` as we don't even have
        // a thread set already.
        //
        // The next thread switch ISR must use the regular path tho:
        scheduler_bootstrapped = true;
        // logk(LOG_DEBUG, "scheduler bootstrapping done...");
    }

    // {
    //     dlist_node_t *iter = thread_wait_queue.head;
    //
    //     logkf(LOG_DEBUG, "queued threads(%{size;x}):", (size_t)&thread_wait_queue);
    //     while (iter != NULL) {
    //
    //         sched_thread_t *const thread = field_parent_ptr(sched_thread_t, schedule_node, iter);
    //
    //         logkf(LOG_DEBUG, "  - %{cs}", sched_get_name(thread));
    //
    //         iter = iter->next;
    //     }
    // }

    uint32_t            task_time_quota  = 0;
    dlist_node_t *const next_thread_node = dlist_pop_front(&thread_wait_queue);

    if (next_thread_node != NULL) {
        sched_thread_t *const next_thread = field_parent_ptr(sched_thread_t, schedule_node, next_thread_node);

        // Set the switch target
        kernel_ctx_switch_set(&next_thread->kernel_ctx);

        task_time_quota = SCHEDULER_MIN_TASK_TIME_US + (uint32_t)next_thread->priority * SCHEDULER_TIME_QUOTA_INCR_US;
        // logkf(LOG_DEBUG, "switch to task '%{cs}'", next_thread->name);

    } else {
        // nothing to do, switch to idle task:

        kernel_ctx_switch_set(&idle_task.kernel_ctx);
        task_time_quota = SCHEDULER_IDLE_TASK_QUOTA_US;
        // logk(LOG_DEBUG, "switch to idle");
    }
    assert_dev_drop(task_time_quota > 0);

    next_isr_invocation_time += task_time_quota;
    time_set_next_task_switch(next_isr_invocation_time);
}

sched_thread_t *sched_create_userland_thread(
    badge_err_t *const            ec,
    process_t *const              process,
    sched_entry_point_t const     entry_point,
    void *const                   arg,
    sched_thread_priority_t const priority
) {
    assert_dev_drop(process != NULL);
    assert_dev_drop(entry_point != NULL);

    sched_thread_t *const thread = thread_alloc();
    if (thread == NULL) {
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOMEM);
        return NULL;
    }

    *thread = (sched_thread_t){
        .process       = process,
        .stack_bottom  = 0x00000000UL,
        .stack_top     = 0x00000000UL,
        .priority      = priority,
        .flags         = 0,
        .schedule_node = DLIST_NODE_EMPTY,
        .exit_code     = 0,
        .kernel_ctx    = {},
    };

    sched_prepare_user_entry(&thread->kernel_ctx, entry_point, arg);

    return thread;
}

sched_thread_t *sched_create_kernel_thread(
    badge_err_t *const            ec,
    sched_entry_point_t const     entry_point,
    void *const                   arg,
    void *const                   stack_bottom,
    size_t const                  stack_size,
    sched_thread_priority_t const priority
) {
    uintptr_t const stack_bottom_addr = (uintptr_t)stack_bottom;

    assert_dev_drop(entry_point != NULL);
    assert_dev_drop(is_aligned(stack_bottom_addr, STACK_ALIGNMENT));
    assert_dev_drop(is_aligned(stack_size, STACK_ALIGNMENT));

    sched_thread_t *const thread = thread_alloc();
    if (thread == NULL) {
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOMEM);
        return NULL;
    }

    *thread = (sched_thread_t){
        .process       = NULL,
        .stack_bottom  = stack_bottom_addr,
        .stack_top     = stack_bottom_addr + stack_size,
        .priority      = priority,
        .flags         = THREAD_KERNEL,
        .schedule_node = DLIST_NODE_EMPTY,
        .exit_code     = 0,
        .kernel_ctx    = {},
    };

    sched_prepare_kernel_entry(&thread->kernel_ctx, thread->stack_top, entry_point, arg);

    return thread;
}

void sched_destroy_thread(badge_err_t *ec, sched_thread_t *thread) {
    assert_dev_drop(thread != NULL);

    if (thread == sched_get_current_thread()) {
        sched_detach_thread(ec, thread);
        if (!badge_err_is_ok(ec)) {
            return;
        }
        sched_exit(0);
    }

    destroy_thread(thread);
    badge_err_set_ok(ec);
}

void sched_detach_thread(badge_err_t *ec, sched_thread_t *thread) {
    assert_dev_drop(thread != NULL);

    enter_critical_section();

    set_flag(thread->flags, THREAD_DETACHED);

    leave_critical_section();

    badge_err_set_ok(ec);
}

void sched_suspend_thread(badge_err_t *const ec, sched_thread_t *const thread) {
    assert_dev_drop(thread != NULL);

    enter_critical_section();

    if (is_flag_set(thread->flags, THREAD_COMPLETED)) {

        // we cannot suspend the thread when it's already completed:
        badge_err_set(ec, ELOC_THREADS, ECAUSE_ILLEGAL);

    } else if (thread == sched_get_current_thread_unsafe()) {

        // Thread currently active. Remove the running flag, and yield.
        // The function will return after resumption of the thread.
        reset_flag(thread->flags, THREAD_RUNNING);
        badge_err_set_ok(ec);

        leave_critical_section();
        sched_yield();
        return;

    } else if (is_flag_set(thread->flags, THREAD_RUNNING)) {

        // thread is currently queued for running. drop it from the queue
        // and remove the running flag.
        reset_flag(thread->flags, THREAD_RUNNING);
        dlist_remove(&thread_wait_queue, &thread->schedule_node);
        badge_err_set_ok(ec);

    } else {

        // thread is neither finished nor running right now, everything is ok:
        badge_err_set_ok(ec);
    }

    leave_critical_section();
}

// Implementation for both `sched_resume_thread` and `sched_resume_thread_next`.
static void sched_resume_thread_inner(
    badge_err_t *const ec, sched_thread_t *const thread, thread_insert_position_t const position
) {
    assert_dev_drop(thread != NULL);
    enter_critical_section();

    if (is_flag_set(thread->flags, THREAD_COMPLETED)) {

        // thread is already completed and cannot be resumed:
        badge_err_set(ec, ELOC_THREADS, ECAUSE_ILLEGAL);
        leave_critical_section();
        return;
    } else if (!is_flag_set(thread->flags, THREAD_RUNNING)) {

        // Thread is not running, but ready to run. Put it into the
        // wait queue and mark it as ready.
        set_flag(thread->flags, THREAD_RUNNING);

        if (position == INSERT_THREAD_FRONT) {
            dlist_prepend(&thread_wait_queue, &thread->schedule_node);
        } else {
            dlist_append(&thread_wait_queue, &thread->schedule_node);
        }

    } else if (position == INSERT_THREAD_FRONT) {
        // Thread is already running and in the wait queue. Remove it and push
        // it to the front:
        assert_dev_drop(dlist_contains(&thread_wait_queue, &thread->schedule_node));

        dlist_remove(&thread_wait_queue, &thread->schedule_node);
        dlist_prepend(&thread_wait_queue, &thread->schedule_node);

        badge_err_set_ok(ec);
    } else {
        // Thread is already running and in the wait queue
        assert_dev_drop(dlist_contains(&thread_wait_queue, &thread->schedule_node));
        badge_err_set_ok(ec);
    }

    leave_critical_section();
}

void sched_resume_thread(badge_err_t *const ec, sched_thread_t *const thread) {
    sched_resume_thread_inner(ec, thread, INSERT_THREAD_BACK);
}

void sched_resume_thread_next(badge_err_t *const ec, sched_thread_t *const thread) {
    sched_resume_thread_inner(ec, thread, INSERT_THREAD_FRONT);
}

process_t *sched_get_associated_process(sched_thread_t const *const thread) {
    enter_critical_section();
    process_t *process = NULL;
    if (thread != NULL) {
        process = thread->process;
    }
    leave_critical_section();
    return process;
}

void sched_yield(void) {
    sched_thread_t *const current_thread = sched_get_current_thread();
    assert_always(current_thread != NULL);

    isr_global_disable();
    sched_request_switch_from_isr();
    isr_context_switch();
}

void sched_exit(uint32_t const exit_code) {
    enter_critical_section();

    sched_thread_t *const current_thread = sched_get_current_thread_unsafe();
    assert_always(current_thread != NULL);

    current_thread->exit_code = exit_code;
    set_flag(current_thread->flags, THREAD_COMPLETED);
    reset_flag(current_thread->flags, THREAD_RUNNING);

    leave_critical_section();

    sched_yield();

    // hint the compiler that we cannot reach this part of the code and
    // it will never be reached:
    __builtin_unreachable();
}


void sched_set_name(badge_err_t *ec, sched_thread_t *thread, char const *name) {
#if NDEBUG
    badge_err_set(ec, ELOC_THREADS, ECAUSE_UNSUPPORTED);
#else
    size_t l = 0;
    for (l = 0; name[l]; l++) {
    }
    if (l + 1 >= sizeof(thread->name)) {
        badge_err_set(ec, ELOC_THREADS, ECAUSE_TOOLONG);
        return;
    }
    mem_copy(thread->name, name, l);
    thread->name[l] = 0;
#endif
}

char const *sched_get_name(sched_thread_t *thread) {
#if NDEBUG
    return "<optimized out>";
#else
    return thread->name;
#endif
}
