
// SPDX-License-Identifier: MIT

#include "port/port.h"

#include "cpulocal.h"
#include "hal/cpu_utility_ll.h"
#include "hal/gpio.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "log.h"
#include "port/pmu_init.h"
#include "rawprint.h"
#include "smp.h"

#include <hal/cpu_utility_ll.h>
#include <hal/pmu_ll.h>
#include <rom/cache.h>
#include <rom/ets_sys.h>
#include <soc/hp_sys_clkrst_struct.h>
#include <soc/interrupts.h>
#include <soc/uart_struct.h>

// CPU0 local data.
cpulocal_t port_cpu0_local;
// CPU1 local data.
cpulocal_t port_cpu1_local = {.cpuid = 1};



void lolfunc() {
    asm(".option push;"
        ".option norelax;"
        "la gp, __global_pointer$;"
        ".option pop;");
    logk_from_isr(LOG_INFO, "This be CPU1");
    while (1) asm("wfi");
}

// Start CPU1.
void port_start_cpu1() {
    cpu_utility_ll_stall_cpu(1);
    HP_SYS_CLKRST.soc_clk_ctrl0.reg_core1_cpu_clk_en = true;
    HP_SYS_CLKRST.hp_rst_en0.reg_rst_en_core1_global = false;
    cpu_utility_ll_reset_cpu(1);
    ets_set_appcpu_boot_addr((uint32_t)&lolfunc);
    cpu_utility_ll_unstall_cpu(1);
}

// Early hardware initialization.
void port_early_init() {
    // Set CPU-local data pointer.
    isr_ctx_get()->cpulocal = &port_cpu0_local;
    // Initialize PMU.
    pmu_init();
}

// Full hardware initialization.
void port_init() {
    // port_start_cpu1();
    // while (1) asm("wfi");
    extern void esp_i2c_isr();
    irq_ch_set_isr(ETS_I2C0_INTR_SOURCE, esp_i2c_isr);
    irq_ch_enable(ETS_I2C0_INTR_SOURCE);
}

// Power off all peripherals.
static void peri_poweroff() {
    // Wait for UART0 to finish sending.
    while (UART0.status.txfifo_cnt) continue;

    // Set all GPIOs to input.
    for (int i = 0; i < io_count(); i++) {
        io_mode(NULL, i, IO_MODE_INPUT);
        io_pull(NULL, i, IO_PULL_NONE);
    }

    // TODO.
}

RAMFUNC static void trigger_restart() {
    ets_set_appcpu_boot_addr(0);

    // Reset other core followed by this core.
    int cpuid = smp_cur_cpu();
    cpu_utility_ll_reset_cpu(!cpuid);
    cpu_utility_ll_reset_cpu(cpuid);
}

RAMFUNC static void enter_deep_sleep() {
    pmu_ll_hp_set_wakeup_enable(&PMU, 0);
    pmu_ll_hp_set_reject_enable(&PMU, 0);

    pmu_ll_hp_clear_wakeup_intr_status(&PMU);
    pmu_ll_hp_clear_reject_intr_status(&PMU);
    pmu_ll_hp_clear_reject_cause(&PMU);

    pmu_ll_hp_set_sleep_enable(&PMU);
}

// Power off.
RAMFUNC void port_poweroff(bool restart) {
    rawprint("\n\n");
    irq_disable();
    peri_poweroff();
    if (restart) {
        trigger_restart();
    } else {
        enter_deep_sleep();
    }
    while (1) continue;
}

// Send a single character to the log output.
void port_putc(char msg) {
    UART0.fifo.val = msg;
}

// Fence data and instruction memory for executable mapping.
void port_fencei() {
    asm("fence rw,rw");
    Cache_WriteBack_All(CACHE_MAP_L1_DCACHE);
    asm("fence.i");
}
