
// SPDX-License-Identifier: MIT

#pragma once

#include "port/interrupt.h"

// Initialise interrupt drivers for this CPU.
void irq_init();

// Enable/disable an external interrupt.
// Returns whether the interrupt was enabled.
bool irq_ext_enable(int ext_irq, bool enable);
// Query whether an external interrupt is enabled.
bool irq_ext_enabled(int ext_irq);
// Route an external interrupt to an internal interrupt.
void irq_route(int ext_irq, int int_irq);

// Enable/disable an internal interrupt.
// Returns whether the interrupt was enabled.
static inline bool irq_enable(int int_irq, bool enable);
// Query whether an internal interrupt is enabled.
static inline bool irq_enabled(int int_irq);

// Globally enable/disable interrupts in this CPU.
// Returns whether interrupts were enabled.
static inline bool irq_global_enable(bool enable);
// Query whether interrupts are enabled in this CPU.
static inline bool irq_global_enabled();
