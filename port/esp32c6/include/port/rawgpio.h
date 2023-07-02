
// SPDX-License-Identifier: MIT

#pragma once

#include <gpio.h>

// Enable or disable "claimed by peripheral" mode for GPIO `pin`.
void rawgpio_set_peripheral	(badge_err_t *ec, int pin, bool claim_as_peripheral);
// Route GPIO `pin` to input signal `signo`.
void rawgpio_route_input	(badge_err_t *ec, int pin, int signo);
// Route GPIO `pin` to output signal `signo`.
void rawgpio_route_output	(badge_err_t *ec, int pin, int signo);
// Disable the signal routing and turn GPIO `pin` into a regular GPIO.
void rawgpio_unroute		(badge_err_t *ec, int pin);
