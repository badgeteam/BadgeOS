
// SPDX-License-Identifier: MIT

#pragma once

#include "badge_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Determines whether an I²C address is valid.
static inline bool i2c_is_valid_addr(int slave_id) {
    return slave_id >= 8 && (slave_id & 0xf0) != 0xf0 && slave_id < 1024;
}

// Returns the amount of I²C peripherals present.
// Cannot produce an error.
int i2c_count() CONST;

/*
// Initialises I²C peripheral i2c_num in slave mode with SDA pin sda_pin, SCL pin scl_pin and a write buffer size of at
// least buf_size (the size of the read buffer is managed by i2c_slave_set_read_buf). When initialised as an I²C slave,
// the modes of the SDA and SCL pins are changed automatically. This function may be used again to change the settings
// on an initialised I²C peripheral in slave mode.
void i2c_slave_init(badge_err_t *, int, int, int, size_t);
// De-initialises I²C peripheral i2c_num in slave mode.
void i2c_slave_deinit(badge_err_t *, int);
// Sets the I²C write callback for I²C peripheral i2c_num in slave mode to isr.
// This callback is called when the I²C write starts and again I²C write completes and/or when the write buffer
// saturates.
void i2c_slave_set_write_isr(badge_err_t *, int, i2c_slave_write_isr_t);
// Sets the I²C read callback for I²C peripheral i2c_num in slave mode to isr.
// This callback is called once when the I²C read starts and again when the I²C read completes and/or when the read
// buffer depletes.
void i2c_slave_set_read_isr(badge_err_t *, int, i2c_slave_read_isr_t);
// Sets the I²C read buffer for I²C peripheral i2c_num to buf (length buf_len).
// The I2C slave will only acknowledge reads of up to buf_len bytes.
// The buffer must remain valid until either the I²C slave is de-initialised or the buffer is replaced by another call
// to i2c_slave_set_read_buf, whichever comes first. The position in the read buffer is reset to 0 every time a read is
// initiated.
void i2c_slave_set_read_buf(badge_err_t *, int, uint8_t const *, size_t);
*/

// Initialises I²C peripheral i2c_num in slave mode with SDA pin `sda_pin`, SCL pin `scl_pin` and clock speed/bitrate
// bitrate. When initialised as an I²C master, the modes of the SDA and SCL pins are changed automatically. This
// function may be used again to change the settings on an initialised I²C peripheral in master mode.
void   i2c_master_init(badge_err_t *ec, int i2c_num, int sda_pin, int scl_pin, int32_t bitrate);
// De-initialises I²C peripheral i2c_num in master mode.
void   i2c_master_deinit(badge_err_t *ec, int i2c_num);
// Reads len bytes into buffer buf from I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged read bytes.
size_t i2c_master_read_from(badge_err_t *ec, int i2c_num, int slave_id, void *buf, size_t len);
// Writes len bytes from buffer buf to I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged written bytes.
size_t i2c_master_write_to(badge_err_t *ec, int i2c_num, int slave_id, void const *buf, size_t len);
