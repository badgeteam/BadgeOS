
// SPDX-License-Identifier: MIT

#include "time.h"

#include "arrays.h"
#include "cpulocal.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "scheduler/isr.h"
#include "spinlock.h"
#include "time_private.h"



// Spinlock for list of unclaimed timer tasks.
static spinlock_t tasks_spinlock = SPINLOCK_T_INIT_SHARED;

// Length of timer tasks array.
static size_t        tasks_len;
// Capacity of timer tasks array.
static size_t        tasks_cap;
// Global timer tasks array.
static timertask_t **tasks;


// Timer task counter.
static atomic_int_least64_t taskno_counter;


// Comparator for timer tasks by timestamp.
int timertask_timestamp_cmp(void const *_a, void const *_b) {
    timertask_t const *a = *(void *const *)_a;
    timertask_t const *b = *(void *const *)_b;
    if (a->timestamp < b->timestamp) {
        return 1;
    } else if (a->timestamp > b->timestamp) {
        return -1;
    } else {
        return 0;
    }
}

// Comparator for timer tasks by ID.
int timertask_id_cmp(void const *_a, void const *_b) {
    timertask_t const *a = *(void *const *)_a;
    timertask_t const *b = *(void *const *)_b;
    if (a->taskno < b->taskno) {
        return -1;
    } else if (a->taskno > b->taskno) {
        return 1;
    } else {
        return 0;
    }
}



// Evaluate the timer for this CPU.
static void eval_cpu_timer(time_cpulocal_t *ctx) {
    spinlock_take_shared(&tasks_spinlock);
    if (tasks_len && (ctx->preempt_time <= 0 || tasks[tasks_len - 1]->timestamp < ctx->preempt_time)) {
        // There is a timer task scheduled that will run first.
        ctx->timer_is_preempt = false;
        time_set_cpu_timer(tasks[0]->timestamp);
    } else {
        // No task or the task will run after the preemption.
        ctx->timer_is_preempt = true;
        time_set_cpu_timer(ctx->preempt_time);
    }
    spinlock_release_shared(&tasks_spinlock);
}

// Sets the alarm time when the next task switch should occur.
void time_set_next_task_switch(timestamp_us_t timestamp) {
    time_cpulocal_t *ctx = &isr_ctx_get()->cpulocal->time;
    ctx->preempt_time    = timestamp;
    eval_cpu_timer(ctx);
}

// Attach a task to a timer interrupt.
// The task with the lowest timestamp is likeliest, but not guaranteed, to run first.
// Returns timer ID on success, -1 on failure.
int64_t time_add_async_task(timestamp_us_t timestamp, timer_fn_t callback, void *cookie) {
    // Allocate timer task.
    timertask_t *task = malloc(sizeof(timertask_t));
    if (!task) {
        return -1;
    }
    int64_t taskno  = atomic_fetch_add(&taskno_counter, 1);
    task->taskno    = taskno;
    task->timestamp = timestamp;
    task->callback  = callback;
    task->cookie    = cookie;

    // Interrupts must be disabled while holding spinlock.
    bool ie = irq_disable();

    // Take spinlock while inserting into the list.
    spinlock_take(&tasks_spinlock);

    // Insert into task array.
    if (!array_lencap_sorted_insert(&tasks, sizeof(void *), &tasks_len, &tasks_cap, &task, timertask_timestamp_cmp)) {
        goto error;
    }

    // Release spinlock so it can be re-taken as shared in `eval_cpu_timer`.
    spinlock_release(&tasks_spinlock);

    // Recalculate this CPU's timer.
    time_cpulocal_t *ctx = &isr_ctx_get()->cpulocal->time;
    eval_cpu_timer(ctx);

    // Re-enable interrupts because we're done with the spinlocks.
    irq_enable_if(ie);
    return taskno;

error:
    // Failed to insert into array.
    spinlock_release(&tasks_spinlock);
    irq_enable_if(ie);
    free(task);
    return -1;
}

// Cancel a task created with `time_add_async_task`.
bool time_cancel_async_task(int64_t taskno) {
    // Interrupts must be disabled while holding spinlock.
    bool ie = irq_disable();

    // Take spinlock while removing from the list.
    spinlock_take(&tasks_spinlock);

    bool found = false;
    for (size_t i = 0; i < tasks_len; i++) {
        if (tasks[i]->taskno == taskno) {
            found = true;
            timertask_t *task;
            array_lencap_remove(&tasks, sizeof(void *), &tasks_len, &tasks_cap, &task, i);
            free(task);
            break;
        }
    }

    // Release spinlock so it can be re-taken as shared in `eval_cpu_timer`.
    spinlock_release(&tasks_spinlock);

    // Recalculate this CPU's timer.
    time_cpulocal_t *ctx = &isr_ctx_get()->cpulocal->time;
    eval_cpu_timer(ctx);

    // Re-enable interrupts because we're done with the spinlocks.
    irq_enable_if(ie);
    return found;
}

// Generic timer init after timer-specific init.
void time_init_generic() {
}

// Callback from timer-specific code when the CPU timer fires.
void time_cpu_timer_isr() {
    time_cpulocal_t *ctx = &isr_ctx_get()->cpulocal->time;
    timestamp_us_t   now = time_us();
    if (ctx->timer_is_preempt) {
        // Preemption timer.
        ctx->preempt_time = TIMESTAMP_US_MAX;
        sched_request_switch_from_isr();

    } else {
        timertask_t *task;

        // Check if there is a task that needs to run right now.
        spinlock_take(&tasks_spinlock);
        if (tasks_len && now >= tasks[tasks_len - 1]->timestamp) {
            task = tasks[--tasks_len];
        }
        spinlock_release(&tasks_spinlock);

        // Run the task, if any.
        if (task) {
            task->callback(task->cookie);
            free(task);
        }
    }

    // Re-evaluate this CPU's timer.
    eval_cpu_timer(ctx);
}
