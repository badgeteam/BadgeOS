
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/riscv.h"

#include <stdbool.h>

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
