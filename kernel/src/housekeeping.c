
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
    // Interval for repeating tasks, or <=0 if not repeating.
    timestamp_us_t interval;
    // Unique task ID number.
    int            taskno;
    // Task code.
    hk_task_t      callback;
    // Task argument.
    void          *arg;
} taskent_t;

// Number of tasks.
size_t     queue_len  = 0;
// Capacity for tasks.
size_t     queue_cap  = 0;
// Tasks queue.
taskent_t *queue      = NULL;
// Task ID counter.
int        taskno_ctr = 0;



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



// The housekeeping thread handle.
static tid_t   hk_thread;
// Task mutex.
static mutex_t hk_mtx = MUTEX_T_INIT;

// Runs housekeeping tasks.
int hk_thread_func(void *ignored) {
    (void)ignored;

    while (1) {
        mutex_acquire(NULL, &hk_mtx, TIMESTAMP_US_MAX);
        timestamp_us_t now  = time_us();
        taskent_t      task = {0};

        // Check all tasks.
        while (queue_len && queue[0].next_time <= now) {
            // Run the first task.
            array_remove(queue, sizeof(taskent_t), queue_len, &task, 0);
            assert_dev_drop(task.callback != NULL);
            task.callback(task.taskno, task.arg);

            if (task.interval > 0 && task.next_time <= TIMESTAMP_US_MAX - task.interval) {
                // Repeated tasks get put back into the queue.
                task.next_time += task.interval;
                array_sorted_insert(queue, sizeof(taskent_t), queue_len - 1, &task, hk_task_time_cmp);
            } else {
                // One-time tasks are removed.
                queue_len--;
            }
        }

        mutex_release(NULL, &hk_mtx);
        thread_yield();
    }
}

// Initialize the housekeeping system.
void hk_init() {
    badge_err_t ec;
    hk_thread = thread_new_kernel(&ec, "housekeeping", hk_thread_func, NULL, SCHED_PRIO_NORMAL);
    badge_err_assert_always(&ec);
    thread_resume(&ec, hk_thread);
    badge_err_assert_always(&ec);
}



// Add a one-time task with optional timestamp to the queue.
// This task will be run in the "housekeeping" task.
// Returns the task number.
int hk_add_once(timestamp_us_t time, hk_task_t task, void *arg) {
    return hk_add_repeated(time, 0, task, arg);
}

// Add a repeating task with optional start timestamp to the queue.
// This task will be run in the "housekeeping" task.
// Returns the task number.
int hk_add_repeated(timestamp_us_t time, timestamp_us_t interval, hk_task_t task, void *arg) {
    if (!task) {
        return -1;
    }
    mutex_acquire(NULL, &hk_mtx, TIMESTAMP_US_MAX);

    int       taskno = taskno_ctr;
    taskent_t ent    = {
           .next_time = time,
           .interval  = interval,
           .taskno    = taskno,
           .callback  = task,
           .arg       = arg,
    };

    if (time <= 0) {
        taskno_ctr += array_lencap_insert(&queue, sizeof(taskent_t), &queue_len, &queue_cap, &ent, 0);
    } else {
        taskno_ctr +=
            array_lencap_sorted_insert(&queue, sizeof(taskent_t), &queue_len, &queue_cap, &ent, hk_task_time_cmp);
    }

    mutex_release(NULL, &hk_mtx);
    return taskno;
}

// Cancel a housekeeping task.
void hk_cancel(int taskno) {
    mutex_acquire(NULL, &hk_mtx, TIMESTAMP_US_MAX);
    for (size_t i = 0; i < queue_len; i++) {
        if (queue[i].taskno == taskno) {
            array_lencap_remove(&queue, sizeof(taskent_t), &queue_len, &queue_cap, NULL, i);
            return;
        }
    }
    mutex_release(NULL, &hk_mtx);
}
