
// SPDX-License-Identifier: MIT

#pragma once

#include "port/interrupt.h"

// Interrupt service routine functions.
typedef void (*isr_t)();

// Initialise interrupt drivers for this CPU.
void irq_init();

// Route an external interrupt to an internal interrupt on this CPU.
void irq_ch_route(int ext_irq, int int_irq);
// Set the priority of an internal interrupt on this CPU, if possible.
// 0 is least priority, 255 is most priority on this CPU.
void irq_ch_prio(int int_irq, int prio);
// Acknowledge an interrupt on this CPU.
void irq_ch_ack(int int_irq);

// Enable/disable an internal interrupt on this CPU.
void irq_ch_enable(int int_irq, bool enable);
// Query whether an internal interrupt is enabled on this CPU.
bool irq_ch_enabled(int int_irq);
// Query whether an internal interrupt is pending.
bool irq_ch_pending(int int_irq);
// Query whether an external interrupt is pending.
bool irq_ch_ext_pending(int ext_irq);

// Set the interrupt service routine for an interrupt on this CPU.
void irq_ch_set_isr(int int_irq, isr_t isr);

// Globally enable/disable interrupts on this CPU.
// Returns whether interrupts were enabled.
static inline bool irq_enable(bool enable);
// Query whether interrupts are enabled on this CPU.
static inline bool irq_enabled();
