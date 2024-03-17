
// SPDX-License-Identifier: MIT

#include "interrupt.h"

#include <stdint.h>

// Initialise interrupt drivers for this CPU.
void irq_init() {
}

// Enable/disable an external interrupt.
// Returns whether the interrupt was enabled.
bool irq_ext_enable(int ext_irq, bool enable) {
}

// Query whether an external interrupt is enabled.
bool irq_ext_enabled(int ext_irq) {
}

// Route an external interrupt to an internal interrupt.
void irq_route(int ext_irq, int int_irq) {
}
