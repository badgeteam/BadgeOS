
// SPDX-License-Identifier: MIT

#include "gpio.h"

#include "soc/gpio_ext_struct.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_struct.h"

// Unsafe check if a pin has a function assigned.
static inline bool io_has_function(int pin) {
    return GPIO.func_out_sel_cfg[pin].out_sel != 128;
}



// Sets the mode of GPIO pin `pin` to `mode`.
void io_mode(badge_err_t *ec, int pin, io_mode_t mode) {
    if (pin < 0 || pin >= io_count()) {
        badge_err_set(ec, ELOC_GPIO, ECAUSE_RANGE);
        return;
    } else if (io_has_function(pin)) {
        badge_err_set(ec, ELOC_GPIO, ECAUSE_NOTCONFIG);
        return;
    }
    badge_err_set_ok(ec);

    switch (mode) {
        default: badge_err_set(ec, ELOC_GPIO, ECAUSE_PARAM); break;
        case IO_MODE_HIGH_Z: {
            GPIO.enable.val &= ~(1 << pin);
        } break;
        case IO_MODE_OUTPUT: {
            GPIO.enable.val |= 1 << pin;
        } break;
        case IO_MODE_INPUT: {
            GPIO.enable.val &= ~(1 << pin);
        } break;
    }
}

// Sets the pull resistor behaviour of GPIO pin `pin` to `dir`.
void io_pull(badge_err_t *ec, int pin, io_pull_t dir) {
    if (pin < 0 || pin >= io_count()) {
        badge_err_set(ec, ELOC_GPIO, ECAUSE_RANGE);
        return;
    }
    badge_err_set_ok(ec);

    switch (dir) {
        default: badge_err_set(ec, ELOC_GPIO, ECAUSE_PARAM); break;
        case IO_PULL_NONE: {
            io_mux_gpio_t tmp = IO_MUX.gpio[pin];
            tmp.mcu_wpu       = 0;
            tmp.mcu_wpd       = 0;
            tmp.fun_wpu       = 0;
            tmp.fun_wpd       = 0;
            IO_MUX.gpio[pin]  = tmp;
        } break;
        case IO_PULL_UP: {
            io_mux_gpio_t tmp = IO_MUX.gpio[pin];
            tmp.mcu_wpu       = 1;
            tmp.mcu_wpd       = 0;
            tmp.fun_wpu       = 1;
            tmp.fun_wpd       = 0;
            IO_MUX.gpio[pin]  = tmp;
        } break;
        case IO_PULL_DOWN: {
            io_mux_gpio_t tmp = IO_MUX.gpio[pin];
            tmp.mcu_wpu       = 0;
            tmp.mcu_wpd       = 1;
            tmp.fun_wpu       = 0;
            tmp.fun_wpd       = 1;
            IO_MUX.gpio[pin]  = tmp;
        } break;
    }
}

// Writes level to GPIO pin pin.
void io_write(badge_err_t *ec, int pin, bool level) {
    if (pin < 0 || pin >= io_count()) {
        badge_err_set(ec, ELOC_GPIO, ECAUSE_RANGE);
        return;
    } else if (io_has_function(pin)) {
        badge_err_set(ec, ELOC_GPIO, ECAUSE_NOTCONFIG);
        return;
    }
    badge_err_set_ok(ec);

    if (level) {
        GPIO.out.val |= 1 << pin;
    } else {
        GPIO.out.val &= ~(1 << pin);
    }
}

// Reads logic level value from GPIO pin `pin`.
// Returns false on error.
bool io_read(badge_err_t *ec, int pin) {
    if (pin < 0 || pin >= io_count()) {
        badge_err_set(ec, ELOC_GPIO, ECAUSE_RANGE);
        return false;
    } else if (io_has_function(pin)) {
        badge_err_set(ec, ELOC_GPIO, ECAUSE_NOTCONFIG);
        return false;
    }
    badge_err_set_ok(ec);

    return (GPIO.in.val >> (pin & 31)) & 1;
}

// Determine whether GPIO `pin` is claimed by a peripheral.
// Returns false on error.
bool io_is_peripheral(badge_err_t *ec, int pin) {
    if (pin < 0 || pin >= io_count()) {
        badge_err_set(ec, ELOC_GPIO, ECAUSE_RANGE);
        return false;
    } else {
        badge_err_set_ok(ec);
        return io_has_function(pin);
    }
}
