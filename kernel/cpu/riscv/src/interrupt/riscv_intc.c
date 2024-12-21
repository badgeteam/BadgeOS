
// SPDX-License-Identifier: MIT

#include "cpu/panic.h"
#include "cpu/riscv.h"
#include "interrupt.h"
#include "log.h"

#ifdef CPU_RISCV_ENABLE_SBI_TIME
// Called by the interrupt handler when the CPU-local timer fires.
void riscv_sbi_timer_interrupt();
#endif

void riscv_interrupt_handler() {
    long cause;
    asm("csrr %0, " CSR_CAUSE_STR : "=r"(cause));

    long int_no = cause & RISCV_VT_ICAUSE_MASK;

#ifdef CPU_RISCV_ENABLE_SBI_TIME
    if (int_no == RISCV_INT_SUPERVISOR_TIMER) {
        asm("csrc sie, %0" ::"r"(1 << RISCV_INT_SUPERVISOR_TIMER));
        riscv_sbi_timer_interrupt();
    } else
#endif
    {
        logkf_from_isr(LOG_FATAL, "Unhandled interrupt 0x%{long;x}", cause);
        panic_abort();
    }
}
