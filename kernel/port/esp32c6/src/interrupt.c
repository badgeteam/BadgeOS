
// SPDX-License-Identifier: MIT

#include "interrupt.h"

#include "cpu/isr.h"
#include "cpu/panic.h"
#include "isr_ctx.h"
#include "port/hardware.h"
#include "port/hardware_allocation.h"
#include "soc/interrupts.h"
#include "soc/pcr_struct.h"
#include "soc/plic_struct.h"

#define TIMER_IRQ_CH 16
#define EXT_IRQ_CH   17

typedef struct {
    // Interrupt routing.
    uint32_t route[77];
    // External interrupt status.
    uint32_t status[3];
} intmtx_t;

// NOLINTNEXTLINE
extern intmtx_t INTMTX;

// Temporary interrupt context before scheduler.
static isr_ctx_t tmp_ctx = {.flags = ISR_CTX_FLAG_KERNEL};

// Interrupt service routine table.
static isr_t isr_table[ETS_MAX_INTR_SOURCE] = {0};

void timer_isr_timer_alarm();



// Initialise interrupt drivers for this CPU.
void irq_init() {
    // Initialise PCR.
    PCR.intmtx_conf = (pcr_intmtx_conf_reg_t){
        .intmtx_clk_en = true,
        .intmtx_rst_en = false,
    };

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
    PLIC_MX.int_en    = 0xfffffffe;
    PLIC_MX.int_type  = 0;
    PLIC_MX.int_clear = 0xffffffff;
    PLIC_MX.int_clear = 0;

    // Set default interrupt priorities.
    for (int i = 0; i < 32; i++) {
        PLIC_MX.int_pri[i] = 7;
    }

    // Enable appropriate interrupts.
    asm volatile("csrw mie, %0" ::"r"((1 << TIMER_IRQ_CH) | (1 << EXT_IRQ_CH)));
}

// Callback from ASM to platform-specific interrupt handler.
void riscv_interrupt_handler() {
    // Get interrupt cause.
    long mcause;
    asm("csrr %0, mcause" : "=r"(mcause));
    mcause &= RISCV_VT_ICAUSE_MASK;

    if (mcause == TIMER_IRQ_CH) {
        timer_isr_timer_alarm();
        return;
    }

    for (int i = 0; i < ETS_MAX_INTR_SOURCE / 32; i++) {
        uint32_t pending = INTMTX.status[i];
        while (pending) {
            int lsb_pos  = __builtin_ctz(pending);
            pending     ^= 1 << lsb_pos;
            int irq      = i * 32 + lsb_pos;
            if (irq && irq_ch_is_enabled(irq)) {
                // Jump to ISR.
                if (isr_table[irq]) {
                    isr_table[irq](irq);
                } else {
                    logkf_from_isr(LOG_FATAL, "Unhandled interrupt %{d}", irq);
                    panic_abort();
                }
            }
        }
    }
}



// Set the external interrupt signal for CPU0 timer IRQs.
void set_cpu_timer_irq(int cpu, int timer_irq) {
    (void)cpu;
    INTMTX.route[timer_irq] = TIMER_IRQ_CH;
}

// Enable the IRQ.
void irq_ch_enable(int irq) {
    assert_dev_drop(irq > 0 && irq < ETS_MAX_INTR_SOURCE);
    INTMTX.route[irq] = EXT_IRQ_CH;
}

// Disable the IRQ.
void irq_ch_disable(int irq) {
    assert_dev_drop(irq > 0 && irq < ETS_MAX_INTR_SOURCE);
    INTMTX.route[irq] = 0;
}

// Query whether the IRQ is enabled.
bool irq_ch_is_enabled(int irq) {
    assert_dev_drop(irq > 0 && irq < ETS_MAX_INTR_SOURCE);
    return INTMTX.route[irq] == EXT_IRQ_CH;
}

// Set the interrupt service routine for an interrupt on this CPU.
void irq_ch_set_isr(int irq, isr_t isr) {
    assert_dev_drop(irq > 0 && irq < ETS_MAX_INTR_SOURCE);
    isr_table[irq] = isr;
}
