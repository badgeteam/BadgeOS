
// SPDX-License-Identifier: MIT

#include "interrupt.h"

#include "cpu/isr.h"
#include "cpu/panic.h"
#include "esp_intmtx.h"
#include "isr_ctx.h"
#include "log.h"
#include "port/hardware_allocation.h"

#include <soc/clic_reg.h>
#include <soc/clic_struct.h>
#include <soc/hp_sys_clkrst_struct.h>
#include <soc/interrupt_core0_reg.h>
#include <soc/interrupts.h>
#include <soc/pmu_struct.h>

#define TIMER_IRQ_CH 16
#define EXT_IRQ_CH   17

// Temporary interrupt context before scheduler.
static isr_ctx_t tmp_ctx = {.flags = ISR_CTX_FLAG_KERNEL};

// Interrupt service routine table.
static isr_t isr_table[ETS_MAX_INTR_SOURCE];

// NOLINTNEXTLINE
extern intmtx_t       INTMTX0;
// NOLINTNEXTLINE
extern intmtx_t       INTMTX1;
// NOLINTNEXTLINE
extern clic_dev_t     CLIC;
// NOLINTNEXTLINE
extern clic_ctl_dev_t CLIC_CTL;

// Number of 32-bit banks of interrupts.
#define IRQ_GROUPS ((ETS_MAX_INTR_SOURCE + 31) / 32)

// Interrupt claiming bitmask.
static atomic_int claim_mask[IRQ_GROUPS]  = {0};
// Interrupt enabled bitmask
static atomic_int enable_mask[IRQ_GROUPS] = {0};

// Get INTMTX for this CPU.
static inline intmtx_t *intmtx_local() CONST;
static inline intmtx_t *intmtx_local() {
    long mhartid;
    asm("csrr %0, mhartid" : "=r"(mhartid));
    return mhartid ? &INTMTX1 : &INTMTX0;
}



// Initialise interrupt drivers for this CPU.
void irq_init() {
    long mhartid;
    asm volatile("csrr %0, mhartid" : "=r"(mhartid));
    HP_SYS_CLKRST.soc_clk_ctrl2.reg_intrmtx_apb_clk_en = true;
    HP_SYS_CLKRST.soc_clk_ctrl0.reg_core0_clic_clk_en  = true;
    HP_SYS_CLKRST.soc_clk_ctrl0.reg_core1_clic_clk_en  = true;

    // Install interrupt handler.
    asm volatile("csrw mstatus, 0");
    asm volatile("csrw mtvec, %0" ::"r"((size_t)&riscv_interrupt_vector_table | 1));
    asm volatile("csrw mscratch, %0" ::"r"(&tmp_ctx));

    // Disable all internal interrupts.
    asm volatile("csrw mie, %0" ::"r"((1 << 11) | (1 << 7)));
    asm volatile("csrw mideleg, 0");

    // Enable interrupt matrix.
    // intmtx_local()->clock.clk_en = true;
    CLIC.int_thresh.val = 0;

    // Set defaults for INTMTX.
    intmtx_t *intmtx = intmtx_local();
    for (size_t i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
        intmtx->map[i].val = 0;
    }

    // Set defaults for CLIC.
    uint32_t num_int = CLIC.int_info.num_int;
    for (uint32_t i = 0; i < num_int; i++) {
        CLIC_CTL.irq_ctl[i] = (clic_int_ctl_reg_t){
            .pending   = false,
            .enable    = false,
            .attr_shv  = false,
            .attr_mode = 3,
            .attr_trig = false,
            .ctl       = 127,
        };
    }
    CLIC_CTL.irq_ctl[EXT_IRQ_CH] = (clic_int_ctl_reg_t){
        .pending   = false,
        .enable    = true,
        .attr_shv  = false,
        .attr_mode = 3,
        .attr_trig = false,
        .ctl       = 127,
    };
}


// Enable the IRQ.
void irq_ch_enable(int irq) {
    assert_dev_drop(irq >= 0 && irq < ETS_MAX_INTR_SOURCE);
    atomic_fetch_or(&enable_mask[irq / 32], 1 << (irq % 32));
    INTMTX0.map[irq].map = EXT_IRQ_CH;
    INTMTX1.map[irq].map = EXT_IRQ_CH;
}

// Disable the IRQ.
void irq_ch_disable(int irq) {
    assert_dev_drop(irq >= 0 && irq < ETS_MAX_INTR_SOURCE);
    atomic_fetch_and(&enable_mask[irq / 32], ~(1 << (irq % 32)));
    INTMTX0.map[irq].map = 0;
    INTMTX1.map[irq].map = 0;
}

// Set the external interrupt signal for CPU timer IRQs.
void set_cpu_timer_irq(int cpu, int irq) {
    assert_dev_drop(irq >= 0 && irq < ETS_MAX_INTR_SOURCE);
    if (cpu) {
        INTMTX1.map[irq].map = TIMER_IRQ_CH;
    } else {
        INTMTX0.map[irq].map = TIMER_IRQ_CH;
    }
    CLIC_CTL.irq_ctl[TIMER_IRQ_CH] = (clic_int_ctl_reg_t){
        .pending   = false,
        .enable    = true,
        .attr_shv  = false,
        .attr_mode = 3,
        .attr_trig = false,
        .ctl       = 127,
    };
}

// Query whether the IRQ is enabled.
bool irq_ch_is_enabled(int irq) {
    assert_dev_drop(irq >= 0 && irq < ETS_MAX_INTR_SOURCE);
    return (enable_mask[irq / 32] >> (irq % 32)) & 1;
}

// Set the interrupt service routine for an interrupt on this CPU.
void irq_ch_set_isr(int irq, isr_t isr) {
    assert_dev_drop(irq >= 0 && irq < ETS_MAX_INTR_SOURCE);
    isr_table[irq] = isr;
}

void timer_isr_timer_alarm();

// Callback from ASM to platform-specific interrupt handler.
void riscv_interrupt_handler() {
    long mcause;
    asm volatile("csrr %0, mcause" : "=r"(mcause));
    if ((mcause & RISCV_VT_ICAUSE_MASK) == TIMER_IRQ_CH) {
        timer_isr_timer_alarm();
        return;
    }

    intmtx_t *intmtx = intmtx_local();

    for (size_t i = 0; i < IRQ_GROUPS; i++) {
        uint32_t pending = intmtx->pending[i] & atomic_load(&enable_mask[i]);
        while (pending) {
            int      lsb_pos   = __builtin_ctz(pending);
            uint32_t lsb_mask  = 1 << lsb_pos;
            pending           ^= lsb_mask;
            int irq            = i * 32 + lsb_pos;
            int prev           = atomic_fetch_or(&claim_mask[i], lsb_mask);
            if (!(prev & lsb_mask)) {
                if (!isr_table[irq]) {
                    logkf(LOG_FATAL, "Unhandled interrupt #%{u32;d}", irq);
                    panic_abort();
                } else {
                    isr_table[irq]((int)irq);
                }
                atomic_fetch_and(&claim_mask[i], ~lsb_mask);
            }
        }
    }
}
