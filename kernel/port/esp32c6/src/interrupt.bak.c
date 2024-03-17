
// SPDX-License-Identifier: MIT

#include "port/interrupt.h"

#include "cpu/isr.h"
#include "port/hardware_allocation.h"
#include "port/intmtx.h"
#include "rawprint.h"
#include "time.h"

// Install interrupt and trap handlers.
void interrupt_init(isr_ctx_t *ctx) {
    // Disable interrupts.
    bool mie = interrupt_disable();

    // Disable interrupt delegation.
    asm volatile("csrw mideleg, x0");

    // Set up trap handler (vectored mode; 256 byte-aligned).
    asm volatile("csrw mtvec, %0" ::"r"((size_t)&__interrupt_vector_table | 1));

    // Set up trap context.
    asm volatile("csrw mscratch, %0" ::"r"(ctx));

    // Mark this thread as privileged.
    ctx->is_kernel_thread = true;

    // Configure interrupt matrix.
    intmtx_init();

    intmtx_set_prio(INT_CHANNEL_TIMER_ALARM, INT_PRIO_TIMER_ALARM);
    intmtx_set_prio(INT_CHANNEL_WATCHDOG_ALARM, INT_PRIO_WATCHDOG_ALARM);
    intmtx_set_prio(INT_CHANNEL_UART, INT_PRIO_UART);
    intmtx_set_prio(INT_CHANNEL_I2C, INT_PRIO_I2C);
    intmtx_set_prio(INT_CHANNEL_SPI, INT_PRIO_SPI);

    intmtx_enable(INT_CHANNEL_TIMER_ALARM);
    // intmtx_enable(INT_CHANNEL_WATCHDOG_ALARM);
    // intmtx_enable(INT_CHANNEL_UART);
    // intmtx_enable(INT_CHANNEL_I2C);
    // intmtx_enable(INT_CHANNEL_SPI);

    // Enable interrupt channels.
    asm volatile("csrw mie, %0" ::"r"(
        (1 << INT_CHANNEL_TIMER_ALARM) | (1 << INT_CHANNEL_WATCHDOG_ALARM) | (1 << INT_CHANNEL_UART) |
        (1 << INT_CHANNEL_I2C) | (1 << INT_CHANNEL_SPI)
    ));

    asm volatile("fence");

    // Re-enable interrupts.
    if (mie) {
        interrupt_enable();
    }
}



// Get the priority of an interrupt channel.
int interrupt_channel_get_prio(int channel) {
    return intmtx_get_prio(channel);
}

// Set the priority of an interrupt channel.
void interrupt_channel_set_prio(int channel, int priority) {
    intmtx_set_prio(channel, priority);
}

// Query whether a specific interrupt channel is enabled.
bool interrupt_channel_is_enabled(int channel) {
    return intmtx_is_enabled(channel);
}

// Disable a specific interrupt channel and return whether it was enabled.
bool interrupt_channel_disable(int channel) {
    return intmtx_disable(channel);
}

// Enable a specific interrupt channel.
void interrupt_channel_enable(int channel) {
    intmtx_enable(channel);
}



// Callback from ASM to platform-specific interrupt handler.
void __interrupt_handler() {
    // Get interrupt cause.
    int mcause;
    asm("csrr %0, mcause" : "=r"(mcause));
    mcause &= 31;

    // Jump to ISRs.
    switch (mcause) {
        case INT_CHANNEL_TIMER_ALARM: timer_isr_timer_alarm(); break;
        case INT_CHANNEL_WATCHDOG_ALARM: timer_isr_watchdog_alarm(); break;
        default: __builtin_unreachable();
    }

    // Acknowledge interrupt.
    intmtx_ack(mcause);
}
