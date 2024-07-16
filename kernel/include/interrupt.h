
// SPDX-License-Identifier: MIT

#pragma once

#include "port/interrupt.h"

// Interrupt service routine functions.
typedef void (*isr_t)(int irq, void *cookie);

// Initialise interrupt drivers for this CPU.
void irq_init();

// Enable an interrupt for a specific CPU.
void irq_ch_enable_affine(int irq, int cpu_index);
// Disable an interrupt for a specific CPU.
void irq_ch_disable_affine(int irq, int cpu_index);
// Enable the IRQ.
void irq_ch_enable(int irq);
// Disable the IRQ.
void irq_ch_disable(int irq);
// Query whether the IRQ is enabled.
bool irq_ch_is_enabled(int irq);
// Set the ISR for a certain IRQ.
void irq_ch_set_isr(int irq, isr_t isr, void *cookie);

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
