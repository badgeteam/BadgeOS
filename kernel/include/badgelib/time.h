
// SPDX-License-Identifier: MIT

#pragma once

#include "port/time.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TIMESTAMP_US_MIN INT64_MIN
#define TIMESTAMP_US_MAX INT64_MAX

typedef int64_t timestamp_us_t;

typedef void (*timer_fn_t)(void *cookie);

// Sets the alarm time when the next callback switch should occur.
void           time_set_next_task_switch(timestamp_us_t timestamp);
// Attach a callback to a timer interrupt.
// The callback with the lowest timestamp is likeliest, but not guaranteed, to run first.
// Returns timer ID on success, -1 on failure.
int64_t        time_add_async_task(timestamp_us_t timestamp, timer_fn_t callback, void *cookie);
// Cancel a callback created with `time_add_async_task`.
bool           time_cancel_async_task(int64_t taskno);
// Get current time in microseconds.
timestamp_us_t time_us();
