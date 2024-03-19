
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
    mem_copy(mem, &cmd, sizeof(cmd));
    dlist_append(trans, mem);
    badge_err_set_ok(ec);
    return true;
}



// Create an I²C transaction.
i2c_trans_t i2c_trans_create(badge_err_t *ec, int i2c_num) {
    if (i2c_num < 0 || i2c_num >= i2c_count()) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_NOTFOUND);
    } else {
        badge_err_set_ok(ec);
    }
    return DLIST_EMPTY;
}

// Clean up an I²C transaction.
void i2c_trans_destroy(badge_err_t *ec, i2c_trans_t *trans) {
    while (trans->len) {
        i2c_cmd_t *cmd = (i2c_cmd_t *)dlist_pop_front(trans);
        if (cmd->type == I2C_CMD_WRITE && cmd->length > I2C_SMALL_WRITE_SIZE) {
            free(cmd->data);
        }
        free(cmd);
    }
    badge_err_set_ok(ec);
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
    append(
        ec,
        trans,
        (i2c_cmd_t){
            .type     = I2C_CMD_ADDR,
            .addr     = slave_id,
            .read_bit = read_bit,
        }
    );
}

// Append a write.
// The write data is copied into the transaction context.
void i2c_trans_write(badge_err_t *ec, i2c_trans_t *trans, void const *buf, size_t len) {
    if (len > I2C_SMALL_WRITE_SIZE) {
        i2c_cmd_t cmd = {
            .type   = I2C_CMD_WRITE,
            .length = len,
            .data   = malloc(len),
        };
        if (!cmd.data) {
            badge_err_set(ec, ELOC_I2C, ECAUSE_NOMEM);
            return;
        }
        mem_copy(cmd.data, buf, len);
        if (!append(ec, trans, cmd)) {
            free(cmd.data);
        }
    } else {
        i2c_cmd_t cmd = {
            .type   = I2C_CMD_WRITE,
            .length = len,
            .data   = malloc(len),
        };
        mem_copy(cmd.small_data, buf, len);
        append(ec, trans, cmd);
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
