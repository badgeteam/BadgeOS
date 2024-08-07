
// SPDX-License-Identifier: MIT

#include "port/port.h"

#include "cpulocal.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "port/clkconfig.h"
#include "port/hardware_allocation.h"
#include "port/pmu_init.h"
#include "soc/ext_irq.h"
#include "soc/pcr_struct.h"
#include "soc/uart_struct.h"
#include "soc/usb_serial_jtag_struct.h"
#include "time.h"

#include <stdbool.h>

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
    irq_ch_route(EXT_IRQ_I2C_EXT0_INTR, INT_CHANNEL_I2C);
    irq_ch_set_isr(INT_CHANNEL_I2C, esp_i2c_isr);
    irq_ch_enable(INT_CHANNEL_I2C, true);
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
