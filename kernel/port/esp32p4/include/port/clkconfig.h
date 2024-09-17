
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stdint.h>

// Configure I2C0 clock.
void clkconfig_i2c0(uint32_t freq_hz, bool enable, bool reset);
// Configure SPI2 clock.
void clkconfig_spi2(uint32_t freq_hz, bool enable, bool reset);
// Configure UART0 clock.
void clkconfig_uart0(uint32_t freq_hz, bool enable, bool reset);
