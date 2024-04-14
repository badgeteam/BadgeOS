
// SPDX-License-Identifier: MIT

#include "interrupt.h"

#include "cpu/isr.h"
#include "cpu/panic.h"
#include "esp_intmtx.h"
#include "isr_ctx.h"
#include "log.h"
#include "port/hardware_allocation.h"
#include "soc/clic_reg.h"
#include "soc/clic_struct.h"
#include "soc/hp_sys_clkrst_struct.h"
#include "soc/interrupt_core0_reg.h"
#include "soc/pmu_struct.h"

// Temporary interrupt context before scheduler.
static isr_ctx_t tmp_ctx = {.is_kernel_thread = true};

// Interrupt service routine table.
static isr_t isr_table[32];

// NOLINTNEXTLINE
extern intmtx_t       INTMTX0;
// NOLINTNEXTLINE
extern intmtx_t       INTMTX1;
// NOLINTNEXTLINE
extern clic_dev_t     CLIC;
// NOLINTNEXTLINE
extern clic_ctl_dev_t CLIC_CTL;

// Get INTMTX for this CPU.
static inline intmtx_t *intmtx_local() CONST;
static inline intmtx_t *intmtx_local() {
    long mhartid;
    asm("csrr %0, mhartid" : "=r"(mhartid));
    return mhartid ? &INTMTX1 : &INTMTX0;
}

// Get INTMTX by CPU number.
static inline intmtx_t *intmtx_cpu(int cpu) CONST;
static inline intmtx_t *intmtx_cpu(int cpu) {
    return cpu ? &INTMTX1 : &INTMTX0;
}



// Initialise interrupt drivers for this CPU.
void irq_init() {
    long mhartid;
    asm("csrr %0, mhartid" : "=r"(mhartid));
    logkf_from_isr(LOG_DEBUG, "mhartid %{long;d}", mhartid);

    HP_SYS_CLKRST.soc_clk_ctrl2.reg_intrmtx_apb_clk_en = true;
    HP_SYS_CLKRST.soc_clk_ctrl0.reg_core0_clic_clk_en  = true;

    // Install interrupt handler.
    asm volatile("csrw mstatus, 0");
    asm volatile("csrw mtvec, %0" ::"r"((size_t)&riscv_interrupt_vector_table | 1));
    asm volatile("csrw mscratch, %0" ::"r"(&tmp_ctx));

    // Disable all internal interrupts.
    asm volatile("csrw mie, 0");
    asm volatile("csrw mideleg, 0");

    // Enable interrupt matrix.
    intmtx_local()->clock.clk_en = true;
    CLIC.int_thresh.val          = 0;
}


// Route an external interrupt to an internal interrupt.
void irq_ch_route(int ext_irq, int int_irq) {
    assert_dev_drop(int_irq >= 0 && int_irq < 32);
    assert_dev_drop(ext_irq >= 0 && ext_irq < ETS_MAX_INTR_SOURCE);
    intmtx_local()->map[ext_irq].val = int_irq;
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
    logkf_from_isr(LOG_DEBUG, "Interrupt");

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



// Enable/disable an internal interrupt.
// Returns whether the interrupt was enabled.
bool irq_ch_enable(int int_irq, bool enable) {
    // Get previous value.
    bool prev                           = CLIC_CTL.extirq_ctl[int_irq].enable;
    // Set new enable.
    CLIC_CTL.extirq_ctl[int_irq].enable = enable;
    // Dummy read to force update.
    CLIC_CTL.extirq_ctl[int_irq].val;
    // Return previous value.
    return prev;
}

// Query whether an internal interrupt is enabled.
bool irq_ch_enabled(int int_irq) {
    return CLIC_CTL.extirq_ctl[int_irq].enable;
}

// Query whether an internal interrupt is pending.
bool irq_ch_pending(int int_irq) {
    return CLIC_CTL.extirq_ctl[int_irq].pending;
}
