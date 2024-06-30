
// SPDX-License-Identifier: MIT

#include "hal/gpio.h"

#include "syscall.h"

// Returns the amount of GPIO pins present.
// Cannot produce an error.
int io_count() {
    return syscall_io_count();
}

// Sets the mode of GPIO pin `pin` to `mode`.
void io_mode(badge_err_t *ec, int pin, io_mode_t mode) {
    syscall_io_mode(ec, pin, mode);
}

// Get the mode of GPIO pin `pin`.
io_mode_t io_getmode(badge_err_t *ec, int pin) {
    return syscall_io_getmode(ec, pin);
}

// Sets the pull resistor behaviour of GPIO pin `pin` to `dir`.
void io_pull(badge_err_t *ec, int pin, io_pull_t dir) {
    syscall_io_pull(ec, pin, dir);
}

// Get the  pull resistor behaviour of GPIO pin `pin`.
io_pull_t io_getpull(badge_err_t *ec, int pin) {
    return syscall_io_getpull(ec, pin);
}

// Writes level to GPIO pin pin.
void io_write(badge_err_t *ec, int pin, bool level) {
    syscall_io_write(ec, pin, level);
}

// Reads logic level value from GPIO pin `pin`.
// Returns false on error.
bool io_read(badge_err_t *ec, int pin) {
    return syscall_io_read(ec, pin);
}

// Determine whether GPIO `pin` is claimed by a peripheral.
// Returns false on error.
bool io_is_peripheral(badge_err_t *ec, int pin) {
    return syscall_io_is_peripheral(ec, pin);
}
