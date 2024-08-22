
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/interrupt.h"
#include "soc/interrupts.h"

#define EXT_IRQ_COUNT ETS_MAX_INTR_SOURCE

// Set the external interrupt signal for CPU0 timer IRQs.
void set_cpu0_timer_irq(int timer_irq);
// Set the external interrupt signal for CPU1 timer IRQs.
void set_cpu1_timer_irq(int timer_irq);
