
// SPDX-License-Identifier: MIT

#include "port/port.h"

#include "hal/i2c.h"
#include "housekeeping.h"
#include "port/hardware_allocation.h"
#include "port/pmu_init.h"


// Early hardware initialization.
void port_early_init() {
    // Initialise PMU.
    pmu_init();
}

// Full hardware initialization.
void port_init() {
    // Install I²C driver.
    logk(LOG_INFO, "Installing I²C driver");
    port_i2c_install_isr(INT_CHANNEL_I2C);
    hk_add_repeated(0, 20000, port_i2c_async_cb, NULL);
}
