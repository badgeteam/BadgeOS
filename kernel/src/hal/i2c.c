
// SPDX-License-Identifier: MIT

#include "hal/i2c.h"

#include "badge_strings.h"
#include "malloc.h"

// Append a node to a transaction.
static inline bool append(badge_err_t *ec, i2c_trans_t *trans, i2c_cmd_t cmd) {
    void *mem = malloc(sizeof(i2c_cmd_t));
    if (!mem) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_NOMEM);
        return false;
    }
    cmd.index = 0;
    mem_copy(mem, &cmd, sizeof(cmd));
    dlist_append(&trans->list, mem);
    badge_err_set_ok(ec);
    return true;
}



// Create an I²C transaction.
i2c_trans_t *i2c_trans_create(badge_err_t *ec) {
    i2c_trans_t *mem = malloc(sizeof(i2c_trans_t));
    if (!mem) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_NOMEM);
        return NULL;
    }
    badge_err_set_ok(ec);
    *mem = (i2c_trans_t){
        .list     = DLIST_EMPTY,
        .callback = NULL,
        .cookie   = NULL,
    };
    return mem;
}

// Clean up an I²C transaction.
void i2c_trans_destroy(i2c_trans_t *trans) {
    while (trans->list.len) {
        i2c_cmd_t *cmd = (i2c_cmd_t *)dlist_pop_front(&trans->list);
        if (cmd->type == I2C_CMD_WRITE && cmd->length > I2C_SMALL_WRITE_SIZE) {
            free(cmd->data);
        }
        free(cmd);
    }
    free(trans);
}

// Append a start condition.
void i2c_trans_start(badge_err_t *ec, i2c_trans_t *trans) {
    append(
        ec,
        trans,
        (i2c_cmd_t){
            .type = I2C_CMD_START,
        }
    );
}

// Append a stop condition.
void i2c_trans_stop(badge_err_t *ec, i2c_trans_t *trans) {
    append(
        ec,
        trans,
        (i2c_cmd_t){
            .type = I2C_CMD_STOP,
        }
    );
}

// Append an I²C address.
void i2c_trans_addr(badge_err_t *ec, i2c_trans_t *trans, int slave_id, bool read_bit) {
    if (slave_id < 0 || slave_id > 1023) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_PARAM);
        return;
    }
    append(
        ec,
        trans,
        (i2c_cmd_t){
            .type       = I2C_CMD_ADDR,
            .addr       = slave_id,
            .addr_10bit = slave_id > 127,
            .read_bit   = read_bit,
        }
    );
}

// Append a write.
// The write data is copied into the transaction context.
static bool i2c_trans_write0(badge_err_t *ec, i2c_trans_t *trans, void const *buf, size_t len) {
    if (len > I2C_SMALL_WRITE_SIZE) {
        i2c_cmd_t cmd = {
            .type   = I2C_CMD_WRITE,
            .length = len,
            .data   = malloc(len),
        };
        if (!cmd.data) {
            badge_err_set(ec, ELOC_I2C, ECAUSE_NOMEM);
            return false;
        }
        mem_copy(cmd.data, buf, len);
        if (!append(ec, trans, cmd)) {
            free(cmd.data);
            return false;
        }
        return true;
    } else {
        i2c_cmd_t cmd = {
            .type   = I2C_CMD_WRITE,
            .length = len,
            .data   = malloc(len),
        };
        mem_copy(cmd.small_data, buf, len);
        return append(ec, trans, cmd);
    }
}

// Append a write.
// The write data is copied into the transaction context.
void i2c_trans_write(badge_err_t *ec, i2c_trans_t *trans, void const *buf, size_t len) {
    uint8_t const *ptr = buf;
    for (size_t i = 0; len; i++) {
        size_t max = I2C_LARGE_WRITE_SIZE < len ? I2C_LARGE_WRITE_SIZE : len;
        if (!i2c_trans_write0(ec, trans, ptr, max)) {
            // If appending one of the partial writes fails, discard the others.
            while (i--) {
                i2c_cmd_t *cmd = (i2c_cmd_t *)dlist_pop_back(&trans->list);
                if (cmd->type == I2C_CMD_WRITE && cmd->length > I2C_SMALL_WRITE_SIZE) {
                    free(cmd->data);
                }
                free(cmd);
            }
        }
        ptr += max;
        len -= max;
    }
}

// Append a read.
// The read pointer must remain valid until the transaction is complete.
void i2c_trans_read(badge_err_t *ec, i2c_trans_t *trans, void *buf, size_t len) {
    append(
        ec,
        trans,
        (i2c_cmd_t){
            .type   = I2C_CMD_READ,
            .data   = buf,
            .length = len,
        }
    );
}
