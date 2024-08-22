
// SPDX-License-Identifier: MIT

#include "port/port.h"

#include "cpulocal.h"
#include "hal/cpu_utility_ll.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "log.h"
#include "port/pmu_init.h"
#include "rom/cache.h"
#include "rom/ets_sys.h"
#include "soc/hp_sys_clkrst_struct.h"
#include "soc/interrupts.h"
#include "soc/uart_struct.h"

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
