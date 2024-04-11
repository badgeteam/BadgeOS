
// SPDX-License-Identifier: MIT

#include "interrupt.h"

#include "cpu/isr.h"
#include "cpu/panic.h"
#include "isr_ctx.h"
#include "log.h"
#include "port/hardware_allocation.h"

// Temporary interrupt context before scheduler.
static isr_ctx_t tmp_ctx = {.is_kernel_thread = true};

// Interrupt service routine table.
static isr_t isr_table[32];



// Initialise interrupt drivers for this CPU.
void irq_init() {
    // Install interrupt handler.
    asm volatile("csrw mstatus, 0");
    asm volatile("csrw mtvec, %0" ::"r"(riscv_interrupt_vector_table));
    asm volatile("csrw mscratch, %0" ::"r"(&tmp_ctx));

    // Disable all internal interrupts.
    asm volatile("csrw mie, 0");
    asm volatile("csrw mideleg, 0");
}


// Route an external interrupt to an internal interrupt.
void irq_ch_route(int ext_irq, int int_irq) {
    assert_dev_drop(int_irq > 0 && int_irq < 32);
    assert_dev_drop(ext_irq >= 0 && ext_irq < EXT_IRQ_COUNT);
    logkf_from_isr(LOG_WARN, "TODO: IRQ route %{d} -> %{d}", ext_irq, int_irq);
}

// Set the priority of an internal interrupt, if possible.
// 0 is least priority, 255 is most priority.
void irq_ch_prio(int int_irq, int raw_prio) {
    assert_dev_drop(int_irq > 0 && int_irq < 32);
    if (raw_prio < 0 || raw_prio > 255)
        raw_prio = 127;
    logkf_from_isr(LOG_WARN, "TODO: IRQ prio %{d} = %{d}", int_irq, raw_prio);
}

// Acknowledge an interrupt.
void irq_ch_ack(int int_irq) {
    logkf_from_isr(LOG_WARN, "TODO: IRQ ack %{d}", int_irq);
}

// Set the interrupt service routine for an interrupt on this CPU.
void irq_ch_set_isr(int int_irq, isr_t isr) {
    assert_dev_drop(int_irq > 0 && int_irq < 32);
    isr_table[int_irq] = isr;
}


// Callback from ASM to platform-specific interrupt handler.
void riscv_interrupt_handler() {
    // Get interrupt cause.
    int mcause;
    asm("csrr %0, mcause" : "=r"(mcause));
    mcause &= 31;

    // Jump to ISR.
    if (isr_table[mcause]) {
        isr_table[mcause]();
    } else {
        logkf_from_isr(LOG_FATAL, "Unhandled interrupt %{d}", mcause);
        panic_abort();
    }

    // Acknowledge interrupt.
    irq_ch_ack(mcause);
}
