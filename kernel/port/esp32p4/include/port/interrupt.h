
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/interrupt.h"
#include "soc/interrupts.h"

#define EXT_IRQ_COUNT ETS_MAX_INTR_SOURCE

// Set the external interrupt signal for CPU timer IRQs.
void set_cpu_timer_irq(int cpu, int timer_irq);
