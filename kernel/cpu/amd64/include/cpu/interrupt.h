
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>

// Enable interrupts if a condition is met.
static inline void irq_enable_if(bool enable) {
    // TODO.
    (void)enable;
}

// Disable interrupts if a condition is met.
static inline void irq_disable_if(bool disable) {
    // TODO.
    (void)disable;
}

// Enable interrupts.
static inline void irq_enable() {
    // TODO.
}

// Disable interrupts.
// Returns whether interrupts were enabled.
static inline bool irq_disable() {
    // TODO.
    return 0;
}

// Query whether interrupts are enabled in this CPU.
static inline bool irq_is_enabled() {
    // TODO.
    return 0;
}
