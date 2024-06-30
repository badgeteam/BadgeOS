
// SPDX-License-Identifier: MIT

#include "hal/spi.h"

#include "syscall.h"

// Returns the amount of SPI peripherals present.
// Cannot produce an error.
int spi_count() {
    return syscall_spi_count();
}

// Initialises SPI peripheral spi_num in controller mode with SCLK pin `sclk_pin`, MOSI pin `mosi_pin`, MISO pin
// `miso_pin`, SS pin `ss_pin` and clock speed/bitrate bitrate. The modes of the SCLK, MOSI, MISO and SS pins are
// changed automatically. This function may be used again to change the settings on an initialised SPI peripheral in
// controller mode.
void spi_controller_init(
    badge_err_t *ec, int spi_num, int sclk_pin, int mosi_pin, int miso_pin, int ss_pin, int32_t bitrate
) {
    syscall_spi_controller_init(ec, spi_num, sclk_pin, mosi_pin, miso_pin, ss_pin, bitrate);
}

// De-initialises SPI peripheral.
void spi_deinit(badge_err_t *ec, int spi_num) {
    syscall_spi_deinit(ec, spi_num);
}

// Reads len bytes into buffer buf from SPI device.
// This function blocks until the entire transaction is completed.
void spi_controller_read(badge_err_t *ec, int spi_num, void *buf, size_t len) {
    syscall_spi_controller_read(ec, spi_num, buf, len);
}

// Writes len bytes from buffer buf to SPI device.
// This function blocks until the entire transaction is completed.
void spi_controller_write(badge_err_t *ec, int spi_num, void const *buf, size_t len) {
    syscall_spi_controller_write(ec, spi_num, buf, len);
}

// Writes len bytes from buffer buf to SPI device, then reads len bytes into buffer buf from SPI device.
// Write and read happen simultaneously if the full-duplex flag fdx is set.
// This function blocks until the entire transaction is completed.
void spi_controller_transfer(badge_err_t *ec, int spi_num, void *buf, size_t len, bool fdx) {
    syscall_spi_controller_transfer(ec, spi_num, buf, len, fdx);
}
