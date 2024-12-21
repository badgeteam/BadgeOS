
// SPDX-License-Identifier: MIT

#include "time.h"

#include "interrupt.h"
#include "port/hardware_allocation.h"
#include "scheduler/isr.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"



// Initialise timer and watchdog subsystem.
void time_init() {
}

// Sets the alarm time when the next task switch should occur.
void time_set_next_task_switch(timestamp_us_t timestamp) {
}

// Attach a task to a timer interrupt.
int64_t time_add_async_task(timestamp_us_t timestamp, timer_fn_t task, void *cookie) {
    return false;
}

// Cancel a task created with `time_add_async_task`.
bool time_cancel_async_task(int64_t taskno) {
    return false;
}

// Get current time in microseconds.
timestamp_us_t time_us() {
    return 0;
}
