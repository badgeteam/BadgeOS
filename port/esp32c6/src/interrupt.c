
// SPDX-License-Identifier: MIT

#include "port/interrupt.h"

#include "cpu/isr.h"
#include "log.h"
#include "port/intmtx.h"
#include "time.h"


// Install interrupt and trap handlers.
void interrupt_init(kernel_ctx_t *ctx) {
    // Disable interrupts.
    long mstatus;
    asm volatile("csrrc %0, mstatus, %1" : "=r"(mstatus) : "r"(0x0000000f));

    // Disable interrupt delegation.
    asm volatile("csrw mideleg, %0" ::"r"(0x00000000));

    // Set up trap handler (vectored mode; 256 byte-aligned).
    asm volatile("csrw mtvec, %0" ::"r"((size_t)&__interrupt_vector_table | 1));

    // Set up trap context.
    asm volatile("csrw mscratch, %0" ::"r"(ctx));

    // Configure interrupt matrix.
    intmtx_init();
    asm volatile("fence");

    // Re-enable interrupts.
    asm volatile("csrs mstatus, %0" ::"r"(mstatus & 0x00000008));
}

// Callback from ASM to platform-specific interrupt handler.
void __interrupt_handler() {
    // Get interrupt cause.
    uint32_t mcause;
    asm("csrr %0, mcause" : "=r"(mcause));
    mcause &= 31;

    // logkf(LOG_DEBUG, "Interrupt %{u32;d}", mcause);

    // Jump to ISRs.
    switch (mcause) {
        case INT_TIMER_ALARM_CH: timer_isr_timer_alarm(); break;
        case INT_WATCHDOG_ALARM_CH: timer_isr_watchdog_alarm(); break;
    }

    // Acknowledge interrupt.
    intmtx_ack(mcause);
}
