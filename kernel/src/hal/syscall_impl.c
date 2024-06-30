
// SPDX-License-Identifier: MIT

#include "hal/gpio.h"
#include "hal/i2c.h"
#include "hal/spi.h"
#include "syscall_util.h"



// Returns the amount of GPIO pins present.
// Cannot produce an error.
int syscall_io_count() {
    return io_count();
}

// Sets the mode of GPIO pin `pin` to `mode`.
void syscall_io_mode(badge_err_t *ec, int pin, io_mode_t mode) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    io_mode(ec, pin, mode);
}
// Get the mode of GPIO pin `pin`.
io_mode_t syscall_io_getmode(badge_err_t *ec, int pin) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    return io_getmode(ec, pin);
}
// Sets the pull resistor behaviour of GPIO pin `pin` to `dir`.
void syscall_io_pull(badge_err_t *ec, int pin, io_pull_t dir) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    return io_pull(ec, pin, dir);
}
// Get the  pull resistor behaviour of GPIO pin `pin`.
io_pull_t syscall_io_getpull(badge_err_t *ec, int pin) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    return io_getpull(ec, pin);
}
// Writes level to GPIO pin pin.
void syscall_io_write(badge_err_t *ec, int pin, bool level) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    return io_write(ec, pin, level);
}
// Reads logic level value from GPIO pin `pin`.
// Returns false on error.
bool syscall_io_read(badge_err_t *ec, int pin) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    return io_read(ec, pin);
}
// Determine whether GPIO `pin` is claimed by a peripheral.
// Returns false on error.
bool syscall_io_is_peripheral(badge_err_t *ec, int pin) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    return io_is_peripheral(ec, pin);
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
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    i2c_master_init(ec, i2c_num, sda_pin, scl_pin, bitrate);
    badge_err_log_warn(ec);
}
// De-initialises I²C peripheral i2c_num in master mode.
void syscall_i2c_master_deinit(badge_err_t *ec, int i2c_num) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    i2c_master_deinit(ec, i2c_num);
    badge_err_log_warn(ec);
}
// Reads len bytes into buffer buf from I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged read bytes.
size_t syscall_i2c_master_read_from(badge_err_t *ec, int i2c_num, int slave_id, void *buf, size_t len) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    sysutil_memassert_rw(buf, len);
    size_t rv = i2c_master_read_from(ec, i2c_num, slave_id, buf, len);
    badge_err_log_warn(ec);
    return rv;
}
// Writes len bytes from buffer buf to I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged written bytes.
size_t syscall_i2c_master_write_to(badge_err_t *ec, int i2c_num, int slave_id, void const *buf, size_t len) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    sysutil_memassert_r(buf, len);
    size_t rv = i2c_master_write_to(ec, i2c_num, slave_id, buf, len);
    badge_err_log_warn(ec);
    return rv;
}



#ifdef BADGEROS_PORT_esp32p4
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
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    spi_controller_init(ec, spi_num, sclk_pin, mosi_pin, miso_pin, ss_pin, bitrate);
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
    sysutil_memassert_rw(buf, len);
    spi_controller_read(ec, spi_num, buf, len);
}
// Writes len bytes from buffer buf to SPI device.
// This function blocks until the entire transaction is completed.
void syscall_spi_controller_write(badge_err_t *ec, int spi_num, void const *buf, size_t len) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    sysutil_memassert_r(buf, len);
    spi_controller_write(ec, spi_num, buf, len);
}
// Writes len bytes from buffer buf to SPI device, then reads len bytes into buffer buf from SPI device.
// Write and read happen simultaneously if the full-duplex flag fdx is set.
// This function blocks until the entire transaction is completed.
void syscall_spi_controller_transfer(badge_err_t *ec, int spi_num, void *buf, size_t len, bool fdx) {
    if (ec) {
        sysutil_memassert_rw(ec, sizeof(badge_err_t));
    }
    sysutil_memassert_rw(buf, len);
    spi_controller_transfer(ec, spi_num, buf, len, fdx);
}
#endif
