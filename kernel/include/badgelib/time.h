
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TIMESTAMP_US_MIN INT64_MIN
#define TIMESTAMP_US_MAX INT64_MAX

typedef int64_t timestamp_us_t;

typedef void (*timer_fn_t)(void *cookie);

// Initialise timer and watchdog subsystem.
void           time_init();
// Sets the alarm time when the next task switch should occur.
void           time_set_next_task_switch(timestamp_us_t timestamp);
// Attach a task to a timer interrupt.
int64_t        time_add_async_task(timestamp_us_t timestamp, timer_fn_t task, void *cookie);
// Cancel a task created with `time_add_async_task`.
bool           time_cancel_async_task(int64_t taskno);
// Get current time in microseconds.
timestamp_us_t time_us();
