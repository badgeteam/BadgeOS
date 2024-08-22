
// SPDX-License-Identifier: MIT

#include "port/port.h"

#include "attributes.h"
#include "cpulocal.h"
#include "hal/gpio.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "port/clkconfig.h"
#include "port/hardware_allocation.h"
#include "port/pmu_init.h"
#include "rawprint.h"
#include "time.h"

#include <stdbool.h>

// NOLINTBEGIN
#define STDLIB_H
#define _STDLIB_H
#define __STDLIB_H
// NOLINTEND

#include <hal/clk_tree_ll.h>
#include <hal/pmu_ll.h>
#include <hal/rwdt_ll.h>
#include <rom/cache.h>
#include <soc/lp_wdt_struct.h>
#include <soc/pcr_struct.h>
#include <soc/timer_group_struct.h>
#include <soc/uart_struct.h>
#include <soc/usb_serial_jtag_struct.h>

cpulocal_t port_cpu_local;



// Early hardware initialization.
void port_early_init() {
    isr_ctx_get()->cpulocal = &port_cpu_local;
    // Initialise PMU.
    pmu_init();
    // Power up UART.
    PCR.uart0_pd_ctrl.uart0_mem_force_pd = false;
    PCR.uart0_pd_ctrl.uart0_mem_force_pu = true;
    PCR.uart0_conf.uart0_rst_en          = false;
    PCR.uart0_conf.uart0_clk_en          = true;
}

// Full hardware initialization.
void port_init() {
    extern void esp_i2c_isr();
    irq_ch_set_isr(ETS_I2C_EXT0_INTR_SOURCE, (isr_t)esp_i2c_isr);
    irq_ch_enable(ETS_I2C_EXT0_INTR_SOURCE);
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

    // Power off UART.
    PCR.uart0_pd_ctrl.uart0_mem_force_pd = false;
    PCR.uart0_pd_ctrl.uart0_mem_force_pu = false;
    PCR.uart0_conf.uart0_rst_en          = true;
    PCR.uart0_conf.uart0_clk_en          = false;
    PCR.uart0_sclk_conf.uart0_sclk_en    = false;

    PCR.uart1_pd_ctrl.uart1_mem_force_pd = false;
    PCR.uart1_pd_ctrl.uart1_mem_force_pu = false;
    PCR.uart1_conf.uart1_rst_en          = true;
    PCR.uart1_conf.uart1_clk_en          = false;
    PCR.uart1_sclk_conf.uart1_sclk_en    = false;

    // Power off I²C.
    PCR.i2c_conf.i2c_clk_en       = false;
    PCR.i2c_conf.i2c_rst_en       = true;
    PCR.i2c_sclk_conf.i2c_sclk_en = false;

    // Power off SPI.
    PCR.spi2_conf.spi2_clk_en       = false;
    PCR.spi2_conf.spi2_rst_en       = false;
    PCR.spi2_clkm_conf.spi2_clkm_en = false;

    // Power off timers.
    PCR.timergroup0_timer_clk_conf.tg0_timer_clk_en = false;
    PCR.timergroup0_wdt_clk_conf.tg0_wdt_clk_en     = false;
    PCR.timergroup0_conf.tg0_clk_en                 = false;
    PCR.timergroup0_conf.tg0_rst_en                 = true;
    PCR.timergroup1_timer_clk_conf.tg1_timer_clk_en = false;
    PCR.timergroup1_wdt_clk_conf.tg1_wdt_clk_en     = false;
    PCR.timergroup1_conf.tg1_clk_en                 = false;
    PCR.timergroup1_conf.tg1_rst_en                 = true;
}

RAMFUNC static void trigger_restart() {
    Cache_Disable_ICache();
    software_reset_cpu(0);
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
    static bool    discon   = false;
    timestamp_us_t timeout  = time_us() + 5000;
    discon                 &= !USB_SERIAL_JTAG.ep1_conf.serial_in_ep_data_free;
    while (!discon && !USB_SERIAL_JTAG.ep1_conf.serial_in_ep_data_free) {
        if (time_us() > timeout)
            discon = true;
    }
    USB_SERIAL_JTAG.ep1.val      = msg;
    USB_SERIAL_JTAG.ep1_conf.val = 1;
    UART0.fifo.val               = msg;
}
