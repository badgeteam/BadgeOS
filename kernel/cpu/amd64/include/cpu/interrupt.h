
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



// AMD64 IDT entry.
typedef __uint128_t x86_idtent_t;

// IDT entry flag: Present.
#define IDT_FLAG_P         ((__uint128_t)1 << 47)
// IDT entry flag: Privilege levels that are allowed to call this interrupt.
#define IDT_FLAG_DPL_BASE  45
// IDT entry flag: Privilege levels that are allowed to call this interrupt.
#define IDT_FLAG_DPL_MASK  ((__uint128_t)3 << IDT_FLAG_DPL_MASK)
// IDT entry flag: Gate type.
#define IDT_FLAG_GATE_MASK ((__uint128_t)15 << 40)
// Gate type: Interrupt gate.
#define IDT_FLAG_GATE_INT  ((__uint128_t)14 << 40)
// Gate type: Trap gate.
#define IDT_FLAG_GATE_TRAP ((__uint128_t)15 << 40)
// IDT entry field: Interrupt stack table.
#define IDT_FLAG_IST_BASE  32
// IDT entry field: Interrupt stack table.
#define IDT_FLAG_IST_MASK  ((__uint128_t)15 << IDT_FLAG_IST_BASE)

// Format an IDT entry.
#define FORMAT_IDTENT(offset, segment, priv, is_int, ist)                                                              \
    (IDT_FLAG_P | ((priv) << IDT_FLAG_DPL_BASE) | ((segment) << 16) | ((offset) & 0xffff) |                            \
     (((__uint128_t)(offset) & 0xffffffffffff0000) << 32))



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
