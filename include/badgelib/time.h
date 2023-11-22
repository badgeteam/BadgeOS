
// SPDX-License-Identifier: MIT

#pragma once

#include "port/hardware.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FREQUENCY_HZ_MIN INT32_MIN
#define FREQUENCY_HZ_MAX INT32_MAX
#define TIMESTAMP_US_MIN INT64_MIN
#define TIMESTAMP_US_MAX INT64_MAX
#define TIMER_VALUE_MIN  INT64_MIN
#define TIMER_VALUE_MAX  INT64_MAX

typedef int32_t frequency_hz_t;
typedef int64_t timestamp_us_t;
typedef int64_t timer_value_t;

// Initialise timer and watchdog subsystem.
void           time_init();
// Get current time in microseconds.
timestamp_us_t time_us();
// Sets the alarm time when the next task switch should occur.
void           time_set_next_task_switch(timestamp_us_t timestamp);

// Get the number of hardware timers.
#define timer_count() (2)
// Set the counting frequency of a hardware timer.
void timer_set_freq(int timerno, frequency_hz_t frequency);
// Configure timer interrupt settings.
void timer_int_config(int timerno, bool enable, int channel);


// Configure timer alarm.
void          timer_alarm_config(int timerno, timer_value_t threshold, bool reset_on_alarm);
// Get the current value of timer.
timer_value_t timer_value_get(int timerno);
// Set the current value of timer.
void          timer_value_set(int timerno, timer_value_t value);
// Enable the timer counting.
void          timer_start(int timerno);
// Disable the timer counting.
void          timer_stop(int timerno);

// Callback to the timer driver for when a timer alarm fires.
void timer_isr_timer_alarm();
// Callback to the timer driver for when a watchdog alarm fires.
void timer_isr_watchdog_alarm();
