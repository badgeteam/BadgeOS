
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/isr_ctx.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Most prioritised interrupt.
#define INT_PRIO_MOST  15
// Least prioritised interrupt.
#define INT_PRIO_LEAST 1

// Install interrupt and trap handlers.
// Requires a preallocated context and regs struct.
void interrupt_init(isr_ctx_t *ctx);

// Get the priority of an interrupt channel.
int  interrupt_channel_get_prio(int channel);
// Set the priority of an interrupt channel.
void interrupt_channel_set_prio(int channel, int priority);
// Query whether a specific interrupt channel is enabled.
bool interrupt_channel_is_enabled(int channel);
// Disable a specific interrupt channel and return whether it was enabled.
bool interrupt_channel_disable(int channel);
// Enable a specific interrupt channel.
void interrupt_channel_enable(int channel);

// Query whether interrupts are enabled.
static inline bool interrupt_is_enabled() {
    uint32_t mstatus;
    asm("csrr %0, mstatus" : "=r"(mstatus));
    return mstatus & 8;
}
// Disable interrupts and return whether they were enabled.
static inline bool interrupt_disable() {
    uint32_t mstatus;
    asm volatile("csrrc %0, mstatus, %1" : "=r"(mstatus) : "r"(8));
    return mstatus & 8;
}
// Enable interrupts.
static inline void interrupt_enable() {
    asm volatile("csrs mstatus, %0" ::"r"(8));
}
