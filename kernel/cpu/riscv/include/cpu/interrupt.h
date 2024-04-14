
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/riscv.h"

#include <stdbool.h>

// Enable/disable an internal interrupt.
// Returns whether the interrupt was enabled.
static inline bool irq_ch_enable(int int_irq, bool enable) {
    long mask = 1 << int_irq;
    if (enable) {
        asm volatile("csrrs %0, mie, %0" : "+r"(mask));
    } else {
        asm volatile("csrrc %0, mie, %0" : "+r"(mask));
    }
    return ((mask) >> int_irq) & 1;
}

// Query whether an internal interrupt is enabled.
static inline bool irq_ch_enabled(int int_irq) {
    long mask;
    asm("csrr %0, mie" : "=r"(mask));
    return ((mask) >> int_irq) & 1;
}

// Globally enable/disable interrupts in this CPU.
// Returns whether interrupts were enabled.
static inline bool irq_enable(bool enable) {
    long mask = 1 << RISCV_STATUS_MIE_BIT;
    if (enable) {
        asm volatile("csrrs %0, mstatus, %0" : "+r"(mask));
    } else {
        asm volatile("csrrc %0, mstatus, %0" : "+r"(mask));
    }
    return (mask >> RISCV_STATUS_MIE_BIT) & 1;
}
// Query whether interrupts are enabled in this CPU.
static inline bool irq_enabled() {
    long mask;
    asm("csrr %0, mstatus" : "=r"(mask));
    return (mask >> RISCV_STATUS_MIE_BIT) & 1;
}
