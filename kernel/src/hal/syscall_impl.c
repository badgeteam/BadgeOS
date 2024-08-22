
// SPDX-License-Identifier: MIT

#include "hal/gpio.h"
#include "hal/i2c.h"
#include "hal/spi.h"
#include "malloc.h"
#include "process/process.h"
#include "process/sighandler.h"
#include "syscall_util.h"
#include "usercopy.h"

#include <config.h>



// Returns the amount of GPIO pins present.
// Cannot produce an error.
int syscall_io_count() {
    return io_count();
}

// Sets the mode of GPIO pin `pin` to `mode`.
void syscall_io_mode(badge_err_t *ec, int pin, io_mode_t mode) {
    SYSUTIL_EC_WRAPPER_V(io_mode, pin, mode);
}
// Get the mode of GPIO pin `pin`.
io_mode_t syscall_io_getmode(badge_err_t *ec, int pin) {
    return SYSUTIL_EC_WRAPPER(io_mode_t, io_getmode, pin);
}
// Sets the pull resistor behaviour of GPIO pin `pin` to `dir`.
void syscall_io_pull(badge_err_t *ec, int pin, io_pull_t dir) {
    SYSUTIL_EC_WRAPPER_V(io_pull, pin, dir);
}
// Get the  pull resistor behaviour of GPIO pin `pin`.
io_pull_t syscall_io_getpull(badge_err_t *ec, int pin) {
    return SYSUTIL_EC_WRAPPER(io_pull_t, io_getpull, pin);
}
// Writes level to GPIO pin pin.
void syscall_io_write(badge_err_t *ec, int pin, bool level) {
    SYSUTIL_EC_WRAPPER_V(io_write, pin, level);
}
// Reads logic level value from GPIO pin `pin`.
// Returns false on error.
bool syscall_io_read(badge_err_t *ec, int pin) {
    return SYSUTIL_EC_WRAPPER(bool, io_read, pin);
}
// Determine whether GPIO `pin` is claimed by a peripheral.
// Returns false on error.
bool syscall_io_is_peripheral(badge_err_t *ec, int pin) {
    return SYSUTIL_EC_WRAPPER(bool, io_is_peripheral, pin);
}



// Returns the amount of I²C peripherals present.
// Cannot produce an error.
int syscall_i2c_count() {
    return i2c_count();
}

// Initialises I²C peripheral i2c_num in slave mode with SDA pin `sda_pin`, SCL pin `scl_pin` and clock speed/bitrate
// bitrate. When initialised as an I²C master, the modes of the SDA and SCL pins are changed automatically. This
// function may be used again to change the settings on an initialised I²C peripheral in master mode.
void syscall_i2c_master_init(badge_err_t *ec, int i2c_num, int sda_pin, int scl_pin, int32_t bitrate) {
    SYSUTIL_EC_WRAPPER_V(i2c_master_init, i2c_num, sda_pin, scl_pin, bitrate);
}
// De-initialises I²C peripheral i2c_num in master mode.
void syscall_i2c_master_deinit(badge_err_t *ec, int i2c_num) {
    SYSUTIL_EC_WRAPPER_V(i2c_master_deinit, i2c_num);
}
// Reads len bytes into buffer buf from I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged read bytes.
size_t syscall_i2c_master_read_from(badge_err_t *ec, int i2c_num, int slave_id, void *buf, size_t len) {
    void *tmp = malloc(len);
    if (!tmp) {
        badge_err_userset(ec, ELOC_I2C, ECAUSE_NOMEM);
        return 0;
    }
    size_t rv      = SYSUTIL_EC_WRAPPER(size_t, i2c_master_write_to, i2c_num, slave_id, tmp, len);
    bool   copy_ok = copy_to_user(proc_current_pid(), (size_t)buf, tmp, len);
    free(tmp);
    sigsegv_assert(copy_ok, (size_t)buf);
    return rv;
}
// Writes len bytes from buffer buf to I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged written bytes.
size_t syscall_i2c_master_write_to(badge_err_t *ec, int i2c_num, int slave_id, void const *buf, size_t len) {
    void *tmp = malloc(len);
    if (!tmp) {
        badge_err_userset(ec, ELOC_I2C, ECAUSE_NOMEM);
        return 0;
    }
    bool copy_ok = copy_from_user(proc_current_pid(), tmp, (size_t)buf, len);
    if (!copy_ok) {
        free(tmp);
        proc_sigsegv_handler((size_t)buf);
    }
    size_t rv = SYSUTIL_EC_WRAPPER(size_t, i2c_master_write_to, i2c_num, slave_id, tmp, len);
    free(tmp);
    return rv;
}



#ifdef CONFIG_TARGET_esp32p4
// Returns the amount of SPI peripherals present.
// Cannot produce an error.
int syscall_spi_count() {
    return 0;
}

// Initialises SPI peripheral spi_num in controller mode with SCLK pin `sclk_pin`, MOSI pin `mosi_pin`, MISO pin
// `miso_pin`, SS pin `ss_pin` and clock speed/bitrate bitrate. The modes of the SCLK, MOSI, MISO and SS pins are
// changed automatically. This function may be used again to change the settings on an initialised SPI peripheral in
// controller mode.
void syscall_spi_controller_init(
    badge_err_t *ec, int spi_num, int sclk_pin, int mosi_pin, int miso_pin, int ss_pin, int32_t bitrate
) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    (void)spi_num;
    (void)sclk_pin;
    (void)mosi_pin;
    (void)miso_pin;
    (void)ss_pin;
    (void)bitrate;
    badge_err_set(ec, ELOC_SPI, ECAUSE_UNSUPPORTED);
}
// De-initialises SPI peripheral.
void syscall_spi_deinit(badge_err_t *ec, int spi_num) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    // TODO: spi_deinit is unimplemented.
    // spi_deinit(ec, spi_num);
    (void)spi_num;
    badge_err_set_ok(ec);
}
// Reads len bytes into buffer buf from SPI device.
// This function blocks until the entire transaction is completed.
void syscall_spi_controller_read(badge_err_t *ec, int spi_num, void *buf, size_t len) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    (void)spi_num;
    (void)buf;
    (void)len;
    badge_err_set(ec, ELOC_SPI, ECAUSE_UNSUPPORTED);
}
// Writes len bytes from buffer buf to SPI device.
// This function blocks until the entire transaction is completed.
void syscall_spi_controller_write(badge_err_t *ec, int spi_num, void const *buf, size_t len) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    (void)spi_num;
    (void)buf;
    (void)len;
    badge_err_set(ec, ELOC_SPI, ECAUSE_UNSUPPORTED);
}
// Writes len bytes from buffer buf to SPI device, then reads len bytes into buffer buf from SPI device.
// Write and read happen simultaneously if the full-duplex flag fdx is set.
// This function blocks until the entire transaction is completed.
void syscall_spi_controller_transfer(badge_err_t *ec, int spi_num, void *buf, size_t len, bool fdx) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    (void)spi_num;
    (void)buf;
    (void)len;
    (void)fdx;
    badge_err_set(ec, ELOC_SPI, ECAUSE_UNSUPPORTED);
}
#else
// Returns the amount of SPI peripherals present.
// Cannot produce an error.
int syscall_spi_count() {
    return spi_count();
}

// Initialises SPI peripheral spi_num in controller mode with SCLK pin `sclk_pin`, MOSI pin `mosi_pin`, MISO pin
// `miso_pin`, SS pin `ss_pin` and clock speed/bitrate bitrate. The modes of the SCLK, MOSI, MISO and SS pins are
// changed automatically. This function may be used again to change the settings on an initialised SPI peripheral in
// controller mode.
void syscall_spi_controller_init(
    badge_err_t *ec, int spi_num, int sclk_pin, int mosi_pin, int miso_pin, int ss_pin, int32_t bitrate
) {
    SYSUTIL_EC_WRAPPER_V(spi_controller_init, spi_num, sclk_pin, mosi_pin, miso_pin, ss_pin, bitrate);
}
// De-initialises SPI peripheral.
void syscall_spi_deinit(badge_err_t *ec, int spi_num) {
    (void)spi_num;
    badge_err_t ec_buf = {0};
    if (ec) {
        sigsegv_assert(copy_to_user(proc_current_pid(), (size_t)ec, &ec_buf, sizeof(badge_err_t)), (size_t)ec);
    }
    // TODO: spi_deinit is unimplemented.
}
// Reads len bytes into buffer buf from SPI device.
// This function blocks until the entire transaction is completed.
void syscall_spi_controller_read(badge_err_t *ec, int spi_num, void *buf, size_t len) {
    SYSUTIL_EC_WRAPPER_V(spi_controller_read, spi_num, buf, len);
}
// Writes len bytes from buffer buf to SPI device.
// This function blocks until the entire transaction is completed.
void syscall_spi_controller_write(badge_err_t *ec, int spi_num, void const *buf, size_t len) {
    SYSUTIL_EC_WRAPPER_V(spi_controller_write, spi_num, buf, len);
}
// Writes len bytes from buffer buf to SPI device, then reads len bytes into buffer buf from SPI device.
// Write and read happen simultaneously if the full-duplex flag fdx is set.
// This function blocks until the entire transaction is completed.
void syscall_spi_controller_transfer(badge_err_t *ec, int spi_num, void *buf, size_t len, bool fdx) {
    SYSUTIL_EC_WRAPPER_V(spi_controller_transfer, spi_num, buf, len, fdx);
}
#endif
