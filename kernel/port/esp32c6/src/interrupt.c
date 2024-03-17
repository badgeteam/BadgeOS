
// SPDX-License-Identifier: MIT

#include "interrupt.h"

#include "cpu/isr.h"
#include "cpu/panic.h"
#include "isr_ctx.h"
#include "port/hardware_allocation.h"
#include "soc/intpri_struct.h"
#include "soc/plic_struct.h"

typedef struct {
    // Interrupt routing.
    uint32_t route[77];
    // External interrupt status.
    uint32_t status[3];
} intmtx_t;

extern intmtx_t  INTMTX;
static isr_ctx_t tmp_ctx;

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

    // Route external interrupts to channel 0 to disable them.
    for (int i = 0; i < 77; i++) {
        INTMTX.route[i] = 0;
    }

    // Enable all external interrupts.
    INTPRI.core0_cpu_int_thresh.val = 0;
    INTPRI.core0_cpu_int_enable.val = 0xfffffffe;
    INTPRI.core0_cpu_int_clear.val  = 0xffffffff;
    INTPRI.core0_cpu_int_clear.val  = 0;
    PLIC_MX.int_en                  = 0xfffffffe;
    PLIC_MX.int_type                = 0;
    PLIC_MX.int_clear               = 0xffffffff;
    PLIC_MX.int_clear               = 0;

    // Set default interrupt priorities.
    for (int i = 0; i < 32; i++) {
        INTPRI.core0_cpu_int_pri[i].map = 7;
    }
}


// Route an external interrupt to an internal interrupt.
void irq_ch_route(int ext_irq, int int_irq) {
    assert_dev_drop(int_irq > 0 && int_irq < 32);
    assert_dev_drop(ext_irq >= 0 && ext_irq < EXT_IRQ_COUNT);
    INTMTX.route[ext_irq] = int_irq;
}

// Set the priority of an internal interrupt, if possible.
// 0 is least priority, 255 is most priority.
void irq_ch_prio(int int_irq, int raw_prio) {
    assert_dev_drop(int_irq > 0 && int_irq < 32);
    if (raw_prio < 0 || raw_prio > 255)
        raw_prio = 127;
    PLIC_MX.int_pri[int_irq] = raw_prio * 14 / 255 + 1;
}

// Acknowledge an interrupt.
void irq_ch_ack(int int_irq) {
    INTPRI.core0_cpu_int_clear.val = 1 << int_irq;
    INTPRI.core0_cpu_int_clear.val = 0;
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
    asm("ebreak");

    // Jump to ISR.
    if (isr_table[mcause]) {
        isr_table[mcause]();
    } else {
        logkf(LOG_FATAL, "Unhandled interrupt %{d}", mcause);
        panic_abort();
    }

    // Acknowledge interrupt.
    irq_ch_ack(mcause);
}
