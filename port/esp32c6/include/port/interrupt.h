
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/kernel_ctx.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Interrupt channel used for timer alarms.
#define INT_TIMER_ALARM_CH		16
// Interrupt channel used for watchdog alarms.
#define INT_WATCHDOG_ALARM_CH	17

// Install interrupt and trap handlers.
// Requires a preallocated context and regs struct.
void interrupt_init(kernel_ctx_t *ctx, cpu_regs_t *regs);
