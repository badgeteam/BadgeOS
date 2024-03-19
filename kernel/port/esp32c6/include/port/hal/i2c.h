
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Threshold for "small writes" to be optimised into a single write command.
#define I2C_SMALL_WRITE_SIZE 8

// Returns the amount of I²C peripherals present.
// Cannot produce an error.
#define i2c_count() (1)

// Install the I²C ISR.
void port_i2c_install_isr(int channel);
// Asynchrounous I²C management callback.
void port_i2c_async_cb(int taskno, void *arg);
