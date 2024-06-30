
// SPDX-License-Identifier: MIT

#include "hal/gpio.h"

#ifdef BADGEROS_PORT_esp32c6
#include "soc/io_mux_struct.h"
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
// NOLINTBEGIN
#define STDLIB_H
#define _STDLIB_H
#define __STDLIB_H
// NOLINTEND
#include "hal/gpio_ll.h"
#include "soc/gpio_sig_map.h"
#pragma GCC diagnostic pop

// Unsafe check if a pin has a function assigned.
static inline bool io_has_function(int pin) {
    return GPIO.func_out_sel_cfg[pin].out_sel != SIG_GPIO_OUT_IDX;
}



// Returns the amount of GPIO pins present.
// Cannot produce an error.
int io_count() {
    return SOC_GPIO_PIN_COUNT;
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
            gpio_ll_output_disable(&GPIO, pin);
        } break;
        case IO_MODE_OUTPUT: {
            gpio_ll_output_enable(&GPIO, pin);
        } break;
        case IO_MODE_INPUT: {
            gpio_ll_output_disable(&GPIO, pin);
        } break;
    }
}

// Get the mode of GPIO pin `pin`.
io_mode_t io_getmode(badge_err_t *ec, int pin) {
    if (pin < 0 || pin >= io_count()) {
        badge_err_set(ec, ELOC_GPIO, ECAUSE_RANGE);
        return IO_MODE_HIGH_Z;
    }
    badge_err_set_ok(ec);
#if SOC_GPIO_PIN_COUNT > 32
    if (pin > 32) {
        pin -= 32;
        return (GPIO.enable.val >> pin) & 1 ? IO_MODE_OUTPUT : IO_MODE_INPUT;
    } else
#endif
    {
        return (GPIO.enable.val >> pin) & 1 ? IO_MODE_OUTPUT : IO_MODE_INPUT;
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
            gpio_ll_pulldown_dis(&GPIO, pin);
            gpio_ll_sleep_pulldown_dis(&GPIO, pin);
            gpio_ll_pullup_dis(&GPIO, pin);
            gpio_ll_sleep_pullup_dis(&GPIO, pin);
        } break;
        case IO_PULL_UP: {
            gpio_ll_pulldown_dis(&GPIO, pin);
            gpio_ll_sleep_pulldown_dis(&GPIO, pin);
            gpio_ll_pullup_en(&GPIO, pin);
            gpio_ll_sleep_pullup_en(&GPIO, pin);
        } break;
        case IO_PULL_DOWN: {
            gpio_ll_pulldown_en(&GPIO, pin);
            gpio_ll_sleep_pulldown_en(&GPIO, pin);
            gpio_ll_pullup_dis(&GPIO, pin);
            gpio_ll_sleep_pullup_dis(&GPIO, pin);
        } break;
    }
}

// Get the  pull resistor behaviour of GPIO pin `pin`.
io_pull_t io_getpull(badge_err_t *ec, int pin) {
    if (pin < 0 || pin >= io_count()) {
        badge_err_set(ec, ELOC_GPIO, ECAUSE_RANGE);
        return IO_PULL_NONE;
    }
    badge_err_set_ok(ec);
#ifdef BADGEROS_PORT_esp32c6
    io_mux_gpio_t tmp = IO_MUX.gpio[pin];
#endif
#ifdef BADGEROS_PORT_esp32p4
    iomux_gpio_reg_t tmp = IOMUX.gpio[pin];
#endif
    if (tmp.mcu_wpu) {
        return IO_PULL_UP;
    } else if (tmp.mcu_wpd) {
        return IO_PULL_DOWN;
    } else {
        return IO_PULL_NONE;
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

    gpio_ll_set_level(&GPIO, pin, level);
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

    return gpio_ll_get_level(&GPIO, pin);
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
