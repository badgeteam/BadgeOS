
// SPDX-License-Identifier: MIT

#include "housekeeping.h"

#include "arrays.h"
#include "assertions.h"
#include "mutex.h"
#include "scheduler/scheduler.h"
#include "stdatomic.h"

// Housekeeping task entry.
typedef struct taskent_t taskent_t;
typedef struct taskent_t {
    // Next time to start the task.
    timestamp_us_t next_time;
    // Interval for repeating tasks.
    timestamp_us_t interval;
    // Task ID number.
    int            taskno;
    // Task code.
    hk_task_t      callback;
    // Task argument.
    void          *arg;
} taskent_t;

// Number of one-time tasks.
size_t     once_queue_len = 0;
// Capacity for one-time tasks.
size_t     once_queue_cap = 0;
// One-time tasks queue.
taskent_t *once_queue     = NULL;

// Number of repeated tasks.
size_t     repeat_queue_len = 0;
// Capacity repeated tasks.
size_t     repeat_queue_cap = 0;
// Repeated tasks queue.
taskent_t *repeat_queue     = NULL;



// Compare two `taskent_t` time.
int hk_task_time_cmp(void const *a, void const *b) {
    taskent_t const *a_ptr = a;
    taskent_t const *b_ptr = b;
    if (a_ptr->next_time < b_ptr->next_time)
        return -1;
    if (a_ptr->next_time > b_ptr->next_time)
        return 1;
    return 0;
}



// Stack for the housekeeping thread.
static uint32_t        hk_stack[2048];
// The housekeeping thread handle.
static sched_thread_t *hk_thread;
// Task mutex.
static mutex_t         hk_mtx = MUTEX_T_INIT;

// Runs housekeeping tasks.
void hk_thread_func(void *ignored) {
    (void)ignored;

    while (1) {
        mutex_acquire(NULL, &hk_mtx, TIMESTAMP_US_MAX);
        timestamp_us_t now = time_us();
        taskent_t      task;

        // Check one-time tasks
        while (once_queue_len && once_queue[0].next_time <= now) {
            // Remove task from the queue.
            array_lencap_remove(&once_queue, sizeof(*once_queue), &once_queue_len, &once_queue_cap, &task, 0);
            // Run the task.
            task.callback(task.taskno, task.arg);
        }

        // Check repeated tasks.
        while (repeat_queue_len && repeat_queue[0].next_time <= now) {
            // Move the task back in the queue.
            array_remove(repeat_queue, sizeof(*repeat_queue), repeat_queue_len, &task, 0);
            task.next_time += task.interval;
            array_sorted_insert(repeat_queue, sizeof(*repeat_queue), repeat_queue_len, &task, hk_task_time_cmp);
        }

        mutex_release(NULL, &hk_mtx);
        sched_yield();
    }
}

// Initialize the housekeeping system.
void hk_init() {
    badge_err_t ec;
    hk_thread = sched_create_kernel_thread(&ec, hk_thread_func, NULL, hk_stack, sizeof(hk_stack), SCHED_PRIO_NORMAL);
    assert_always(badge_err_is_ok(&ec));
    sched_resume_thread(&ec, hk_thread);
    assert_always(badge_err_is_ok(&ec));
}



// Add a one-time task with optional timestamp to the queue.
// This task will be run in the "housekeeping" task.
// Returns the task number.
int hk_add_once(timestamp_us_t time, hk_task_t task, void *arg) {
    return 0;
}

// Add a repeating task with optional start timestamp to the queue.
// This task will be run in the "housekeeping" task.
// Returns the task number.
int hk_add_repeated(timestamp_us_t time, timestamp_us_t interval, hk_task_t task, void *arg) {
    return 0;
}

// Cancel a housekeeping task.
void hk_cancel(int task) {
}
