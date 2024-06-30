
// SPDX-License-Identifier: MIT

#pragma once

#include "badge_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    // high-impedance; no digital or analog functions
    IO_MODE_HIGH_Z,
    // digital output pin; io_write
    IO_MODE_OUTPUT,
    // digital input pin; optionally with pull resistors; io_read
    IO_MODE_INPUT,
    // PWM-based output pin; io_pwm_write
    IO_MODE_PWM,
    // DAC-based output pin; io_dac_write
    IO_MODE_DAC,
    // ADC-based input pin; optionally with pull resistors; io_adc_read
    IO_MODE_ADC,
} io_mode_t;

typedef enum {
    // no internal pull resistors; floating
    IO_PULL_NONE,
    // internal pull-up resistor; disconnected pins will read VCC
    IO_PULL_UP,
    // internal pull-down resistor; disconnected pins will read 0v
    IO_PULL_DOWN,
} io_pull_t;


// Returns the amount of GPIO pins present.
// Cannot produce an error.
int       io_count() CONST;
// Sets the mode of GPIO pin `pin` to `mode`.
void      io_mode(badge_err_t *ec, int pin, io_mode_t mode);
// Get the mode of GPIO pin `pin`.
io_mode_t io_getmode(badge_err_t *ec, int pin);
// Sets the pull resistor behaviour of GPIO pin `pin` to `dir`.
void      io_pull(badge_err_t *ec, int pin, io_pull_t dir);
// Get the  pull resistor behaviour of GPIO pin `pin`.
io_pull_t io_getpull(badge_err_t *ec, int pin);
// Writes level to GPIO pin pin.
void      io_write(badge_err_t *ec, int pin, bool level);
// Reads logic level value from GPIO pin `pin`.
// Returns false on error.
bool      io_read(badge_err_t *ec, int pin);
// Determine whether GPIO `pin` is claimed by a peripheral.
// Returns false on error.
bool      io_is_peripheral(badge_err_t *ec, int pin);
