
// SPDX-License-Identifier: MIT

#include "port/port.h"

#include "cpulocal.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "log.h"
#include "port/pmu_init.h"
#include "rom/cache.h"
#include "soc/hp_sys_clkrst_struct.h"
#include "soc/interrupts.h"
#include "soc/uart_struct.h"

// CPU0 local data.
cpulocal_t port_cpu0_local;
// CPU1 local data.
cpulocal_t port_cpu1_local;



// Early hardware initialization.
void port_early_init() {
    // Set CPU-local data pointer.
    isr_ctx_get()->cpulocal = &port_cpu0_local;
    // Initialize PMU.
    pmu_init();
}

// Full hardware initialization.
void port_init() {
    extern void esp_i2c_isr();
    irq_ch_route(ETS_I2C0_INTR_SOURCE, INT_CHANNEL_I2C);
    irq_ch_set_isr(INT_CHANNEL_I2C, esp_i2c_isr);
    irq_ch_enable(INT_CHANNEL_I2C, true);
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
