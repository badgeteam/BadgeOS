
// SPDX-License-Identifier: MIT

#include "time.h"

#include "interrupt.h"
#include "port/hardware_allocation.h"
#include "scheduler/isr.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"



// Callback to the timer driver for when a timer alarm fires.
static void timer_isr_timer_alarm() {
    // TODO:
    // timer_alarm_disable(TIMER_SYSTICK_NUM);
    // timer_int_clear(TIMER_SYSTICK_NUM);
    sched_request_switch_from_isr();
}

// Initialise timer and watchdog subsystem.
void time_init() {
}

// Sets the alarm time when the next task switch should occur.
void time_set_next_task_switch(timestamp_us_t timestamp) {
}



// Set timer frequency.
void timer_set_freq(int timerno, frequency_hz_t freq) {
}

// Start timer.
void timer_start(int timerno) {
}

// Stop timer.
void timer_stop(int timerno) {
}

// Configure timer alarm.
void timer_alarm_config(int timerno, int64_t threshold, bool reset_on_alarm) {
}

// Disable timer alarm.
void timer_alarm_disable(int timerno) {
}

// Get timer value.
int64_t timer_value_get(int timerno) {
    return 0;
}

// Set timer value.
void timer_value_set(int timerno, int64_t time) {
}



// Check whether timer has interrupts enabled.
bool timer_int_enabled(int timerno) {
    return false;
}

// Enable / disable timer interrupts.
void timer_int_enable(int timerno, bool enable) {
}

// Check whether timer interrupt had fired.
bool timer_int_pending(int timerno) {
    return false;
}

// Clear timer interrupt.
void timer_int_clear(int timerno) {
}
