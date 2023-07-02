
// SPDX-License-Identifier: MIT

#pragma once

#include "port/hardware.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Initialise timer and watchdog subsystem.
void time_init();
// Get current time in microseconds.
int64_t time_us();

// Get the number of hardware timers.
#define timer_count() (2)
// Set the counting frequency of a hardware timer.
void timer_set_freq(int timerno, int32_t frequency);
// Configure timer interrupt settings.
void timer_int_config(int timerno, bool interrupt_enable, int interrupt_channel);
// Configure timer alarm.
void timer_alarm_config(int timerno, int64_t threshold, bool reset_on_alarm);
// Get the current value of timer.
int64_t timer_value_get(int timerno);
// Set the current value of timer.
void timer_value_set(int timerno, int64_t value);
// Enable the timer counting.
void timer_start(int timerno);
// Disable the timer counting.
void timer_stop(int timerno);

// Callback to the timer driver for when a timer alarm fires.
void timer_isr_timer_alarm();
// Callback to the timer driver for when a watchdog alarm fires.
void timer_isr_watchdog_alarm();
