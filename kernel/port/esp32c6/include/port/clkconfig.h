
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stdint.h>

// Configure DMA clock.
void clkconfig_gmda(bool enable, bool reset);

// Configure I2C0 clock.
void clkconfig_i2c0(uint32_t freq_hz, bool enable, bool reset);
