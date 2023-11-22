
// SPDX-License-Identifier: MIT

#pragma once

#include "time.h"

typedef void (*hk_task_t)(int taskno, void *arg);

// Initialize the housekeeping system.
void hk_init();

// Add a one-time task with optional timestamp to the queue.
// This task will be run in the "housekeeping" task.
// Returns the task number.
int  hk_add_once(timestamp_us_t time, hk_task_t task, void *arg);
// Add a repeating task with optional start timestamp to the queue.
// This task will be run in the "housekeeping" task.
// Returns the task number.
int  hk_add_repeated(timestamp_us_t time, timestamp_us_t interval, hk_task_t task, void *arg);
// Cancel a housekeeping task.
void hk_cancel(int task);
