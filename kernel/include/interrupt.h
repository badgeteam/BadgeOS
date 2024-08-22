
// SPDX-License-Identifier: MIT

#pragma once

#include "port/interrupt.h"

// Interrupt service routine functions.
typedef void (*isr_t)(int irq);

// Initialise interrupt drivers for this CPU.
void irq_init();

// Enable the IRQ.
void irq_ch_enable(int irq);
// Disable the IRQ.
void irq_ch_disable(int irq);
// Query whether the IRQ is enabled.
bool irq_ch_is_enabled(int irq);
// Set the ISR for a certain IRQ.
void irq_ch_set_isr(int irq, isr_t isr);

// Enable interrupts if a condition is met.
static inline void irq_enable_if(bool enable);
// Disable interrupts if a condition is met.
static inline void irq_disable_if(bool disable);
// Enable interrupts.
static inline void irq_enable();
// Disable interrupts.
// Returns whether interrupts were enabled.
static inline bool irq_disable();
// Query whether interrupts are enabled on this CPU.
static inline bool irq_is_enabled();
