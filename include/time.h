
// SPDX-License-Identifier: MIT

#pragma once

#include "port/hardware.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef int32_t frequency_hz_t;
typedef int64_t  timestamp_us_t;
typedef int64_t  timestamp_unspec_t;

enum {
  TIME_MS_PER_S  = 1000U,
  TIME_US_PER_MS = 1000U,
  TIME_US_PER_S  = 1000U * 1000U,
};

// Initialise timer and watchdog subsystem.
void time_init();
// Get current time in microseconds.
timestamp_us_t time_us();

// Get the number of hardware timers.
#define timer_count() (2)
// Set the counting frequency of a hardware timer.
void timer_set_freq(int timerno, frequency_hz_t frequency);
// Configure timer interrupt settings.
void timer_int_config(int timerno, bool enable, int channel);

// Configure timer alarm.
void timer_alarm_config(int timerno, timestamp_unspec_t threshold,
                        bool reset_on_alarm);
// Get the current value of timer.
timestamp_unspec_t timer_value_get(int timerno);
// Set the current value of timer.
void timer_value_set(int timerno, timestamp_unspec_t value);
// Enable the timer counting.
void timer_start(int timerno);
// Disable the timer counting.
void timer_stop(int timerno);

// Callback to the timer driver for when a timer alarm fires.
void timer_isr_timer_alarm();
// Callback to the timer driver for when a watchdog alarm fires.
void timer_isr_watchdog_alarm();
