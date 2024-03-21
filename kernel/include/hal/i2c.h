
// SPDX-License-Identifier: MIT

#pragma once

#include "badge_err.h"
#include "list.h"
#include "port/hal/i2c.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// I²C master command type.
typedef enum {
    // Start condition.
    I2C_CMD_START,
    // Stop condition.
    I2C_CMD_STOP,
    // Slave address.
    I2C_CMD_ADDR,
    // Write data.
    I2C_CMD_WRITE,
    // Read data.
    I2C_CMD_READ,
} i2c_cmd_type_t;

// I²C master command.
typedef struct {
    // Doubly-linked list node.
    dlist_node_t   node;
    // Command type.
    i2c_cmd_type_t type;
    // Read / write length.
    size_t         length;
    // Read / write index for the I²C ISR.
    size_t         index;
    union {
        struct {
            // Slave address.
            uint16_t addr;
            // Slave address is 10-bit.
            bool     addr_10bit;
            // Read bit.
            bool     read_bit;
        };
        // Read / write pointer.
        uint8_t *data;
        // Small write data.
        uint8_t  small_data[I2C_SMALL_WRITE_SIZE];
    };
} i2c_cmd_t;

// I²C transaction finished callback.
// `byte_count` is the number of successfully exchanged bytes.
typedef void (*i2c_trans_cb_t)(badge_err_t status, size_t byte_count, void *cookie);

// I²C master transaction context.
typedef struct {
    dlist_t        list;
    i2c_trans_cb_t callback;
    void          *cookie;
} i2c_trans_t;


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
// This function blocks until the entire transaction is completed returns the number of acknowledged read bytes.
size_t i2c_master_read_from(badge_err_t *ec, int i2c_num, int slave_id, void *buf, size_t len);
// Writes len bytes from buffer buf to I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged written bytes.
size_t i2c_master_write_to(badge_err_t *ec, int i2c_num, int slave_id, void const *buf, size_t len);
// Perform a preconstructed transaction and clean it up afterward.
// Returns how many non-address bytes were exchanged successfully.
size_t i2c_master_run(badge_err_t *ec, int i2c_num, i2c_trans_t *trans);


// Create an I²C transaction.
i2c_trans_t *i2c_trans_create(badge_err_t *ec);
// Clean up an I²C transaction.
void         i2c_trans_destroy(i2c_trans_t *trans);
// Append a start condition.
void         i2c_trans_start(badge_err_t *ec, i2c_trans_t *trans);
// Append a stop condition.
void         i2c_trans_stop(badge_err_t *ec, i2c_trans_t *trans);
// Append an I²C address.
void         i2c_trans_addr(badge_err_t *ec, i2c_trans_t *trans, int slave_id, bool read_bit);
// Append a write.
// The write data is copied into the transaction context.
void         i2c_trans_write(badge_err_t *ec, i2c_trans_t *trans, void const *buf, size_t len);
// Append a read.
// The read pointer must remain valid until the transaction is complete.
void         i2c_trans_read(badge_err_t *ec, i2c_trans_t *trans, void *buf, size_t len);

// Set the on finished callback.
static inline void i2c_trans_set_cb(i2c_trans_t *trans, i2c_trans_cb_t callback, void *cookie) {
    trans->callback = callback;
    trans->cookie   = cookie;
}

// Append a single-byte write.
static inline void i2c_trans_write1(badge_err_t *ec, i2c_trans_t *trans, uint8_t data) {
    i2c_trans_write(ec, trans, &data, 1);
}
