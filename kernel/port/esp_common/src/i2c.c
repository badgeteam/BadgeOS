
// SPDX-License-Identifier: MIT

#include "hal/i2c.h"

#include "badge_err.h"
#include "hal/gpio.h"
#include "interrupt.h"
#include "scheduler/scheduler.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
// NOLINTBEGIN
#define STDLIB_H
#define _STDLIB_H
#define __STDLIB_H
// NOLINTEND
#include "esp_rom_gpio.h"
#include "hal/clk_tree_ll.h"
#include "hal/gpio_hal.h"
#include "hal/gpio_ll.h"
#include "hal/i2c_hal.h"
#include "hal/i2c_ll.h"
#include "soc/gpio_sig_map.h"
#pragma GCC diagnostic pop

#define I2C_ACK  0
#define I2C_NACK 1

#define I2C_DEV   (i2c_dev[i2c_num])
#define I2C_COUNT ((int)SOC_I2C_NUM)



typedef struct {
    uint32_t sda_in_sig;
    uint32_t scl_in_sig;
    uint32_t sda_out_sig;
    uint32_t scl_out_sig;
} i2c_sigtab_t;

typedef struct {
    bool    addr_10bit;
    uint8_t addr_buf[2];
} i2c_fsm_addr_t;

typedef struct {
    uint8_t  opcode;
    bool     ack_check;
    bool     ack_exp;
    bool     ack_resp;
    size_t   len;
    uint8_t *buf;
} i2c_fsm_cmd_t;

typedef struct {
    int            sda_pin;
    int            scl_pin;
    atomic_int     busy;
    bool           is_master;
    bool           enabled;
    i2c_fsm_cmd_t *cmd;
    size_t         cmd_len;
    size_t         cur_cmd;
    size_t         cur_len;
    size_t         txd_cmd;
    size_t         rxd_cmd;
    size_t         txd_len;
    size_t         rxd_len;
    size_t         trans_bytes;
} i2c_ctx_t;



// Returns the amount of I²C peripherals present.
// Cannot produce an error.
int i2c_count() {
    return I2C_COUNT;
}

// I²C dev table.
static i2c_dev_t *const i2c_dev[] = {
    &I2C0,
#if SOC_I2C_NUM > 1
    &I2C1,
#endif
};

// I²C sig table.
static i2c_sigtab_t const i2c_sigtab[] = {
    {
        I2CEXT0_SDA_IN_IDX,
        I2CEXT0_SCL_IN_IDX,
        I2CEXT0_SDA_OUT_IDX,
        I2CEXT0_SCL_OUT_IDX,
    },
#if SOC_I2C_NUM > 1
    {
        I2CEXT1_SDA_IN_IDX,
        I2CEXT1_SCL_IN_IDX,
        I2CEXT1_SDA_OUT_IDX,
        I2CEXT1_SCL_OUT_IDX,
    },
#endif
};

// Current pins in use.
static i2c_ctx_t i2c_ctx[I2C_COUNT] = {0};

static i2c_fsm_addr_t i2c_fmt_addr(int slave_id, bool rw_bit) {
    if (slave_id > 127) {
        return (i2c_fsm_addr_t){true, {0xf0 | ((slave_id >> 7) & 6) | rw_bit, slave_id}};
    } else {
        return (i2c_fsm_addr_t){false, {(slave_id << 1) | rw_bit}};
    }
}



// Queue as much of as many commands as possible.
static void i2c_queue_cmds(int i2c_num) {
    int i;
    for (i = 0; i < SOC_I2C_CMD_REG_NUM && i2c_ctx[i2c_num].cur_cmd < i2c_ctx[i2c_num].cmd_len; i++) {
        i2c_fsm_cmd_t fsm_cmd     = i2c_ctx[i2c_num].cmd[i2c_ctx[i2c_num].cur_cmd];
        size_t        fsm_cmd_len = fsm_cmd.len - i2c_ctx[i2c_num].cur_len;
        uint8_t       byte_num    = fsm_cmd_len <= 255 ? fsm_cmd_len : 255;

        i2c_ll_hw_cmd_t hw_cmd = {
            .byte_num = byte_num,
            .ack_en   = fsm_cmd.ack_check,
            .ack_exp  = fsm_cmd.ack_exp,
            .ack_val  = fsm_cmd.ack_resp,
            .op_code  = fsm_cmd.opcode,
            .done     = false,
        };
        i2c_ll_master_write_cmd_reg(I2C_DEV, hw_cmd, i);

        if (fsm_cmd.opcode == I2C_LL_CMD_WRITE || fsm_cmd.opcode == I2C_LL_CMD_READ) {
            if (fsm_cmd_len <= 255) {
                i2c_ctx[i2c_num].cur_cmd++;
                i2c_ctx[i2c_num].cur_len = 0;
            } else {
                i2c_ctx[i2c_num].cur_len += byte_num;
            }
        } else {
            i2c_ctx[i2c_num].cur_cmd++;
        }
    }
    if (i < SOC_I2C_CMD_REG_NUM) {
        i2c_ll_hw_cmd_t hw_cmd = {0};
        hw_cmd.op_code         = I2C_LL_CMD_END;
        i2c_ll_master_write_cmd_reg(I2C_DEV, hw_cmd, i);
    }
}

// Transmission done ISR.
static void i2c_isr_trans_done(int i2c_num) {
    // Check for any remaining commands.
    if (i2c_ctx[i2c_num].cur_cmd >= i2c_ctx[i2c_num].cmd_len) {
        i2c_ll_clear_intr_mask(I2C_DEV, I2C_LL_INTR_MST_COMPLETE);
        atomic_store(&i2c_ctx[i2c_num].busy, false);
        return;
    }
    // If there are, queue them up.
    i2c_queue_cmds(i2c_num);
    i2c_ll_update(I2C_DEV);
    i2c_ll_master_trans_start(I2C_DEV);
}

// TXFIFO empty ISR.
static void i2c_isr_txfifo(int i2c_num) {
    for (size_t i = 0; i < SOC_I2C_CMD_REG_NUM && i2c_ctx[i2c_num].txd_cmd < i2c_ctx[i2c_num].cmd_len; i++) {
        uint32_t avl;
        i2c_ll_get_txfifo_len(I2C_DEV, &avl);
        i2c_fsm_cmd_t fsm_cmd     = i2c_ctx[i2c_num].cmd[i2c_ctx[i2c_num].txd_cmd];
        size_t        fsm_cmd_len = fsm_cmd.len - i2c_ctx[i2c_num].txd_len;
        uint8_t       byte_num    = fsm_cmd_len <= avl ? fsm_cmd_len : avl;

        if (fsm_cmd.opcode == I2C_LL_CMD_WRITE) {
            i2c_ll_write_txfifo(I2C_DEV, fsm_cmd.buf + i2c_ctx[i2c_num].txd_len, byte_num);
            if (fsm_cmd_len <= avl) {
                i2c_ctx[i2c_num].txd_cmd++;
                i2c_ctx[i2c_num].txd_len = 0;
            } else {
                i2c_ctx[i2c_num].txd_len += byte_num;
            }
        } else {
            i2c_ctx[i2c_num].txd_cmd++;
        }
    }
    if (i2c_ctx[i2c_num].txd_cmd >= i2c_ctx[i2c_num].cmd_len) {
        i2c_ll_disable_intr_mask(I2C_DEV, I2C_INTR_MST_TXFIFO_WM);
    }
    i2c_ll_clear_intr_mask(I2C_DEV, I2C_INTR_MST_TXFIFO_WM);
}

// RXFIFO full ISR.
static void i2c_isr_rxfifo(int i2c_num) {
    for (size_t i = 0; i < SOC_I2C_CMD_REG_NUM && i2c_ctx[i2c_num].rxd_cmd < i2c_ctx[i2c_num].cmd_len; i++) {
        uint32_t avl;
        i2c_ll_get_rxfifo_cnt(I2C_DEV, &avl);
        i2c_fsm_cmd_t fsm_cmd     = i2c_ctx[i2c_num].cmd[i2c_ctx[i2c_num].rxd_cmd];
        size_t        fsm_cmd_len = fsm_cmd.len - i2c_ctx[i2c_num].rxd_len;
        uint8_t       byte_num    = fsm_cmd_len <= avl ? fsm_cmd_len : avl;

        if (fsm_cmd.opcode == I2C_LL_CMD_READ) {
            i2c_ll_read_rxfifo(I2C_DEV, fsm_cmd.buf + i2c_ctx[i2c_num].rxd_len, byte_num);
            if (fsm_cmd_len <= avl) {
                i2c_ctx[i2c_num].rxd_cmd++;
                i2c_ctx[i2c_num].rxd_len = 0;
            } else {
                i2c_ctx[i2c_num].rxd_len += byte_num;
            }
        } else {
            i2c_ctx[i2c_num].rxd_cmd++;
        }
    }
    if (i2c_ctx[i2c_num].rxd_cmd >= i2c_ctx[i2c_num].cmd_len) {
        i2c_ll_disable_intr_mask(I2C_DEV, I2C_INTR_MST_RXFIFO_WM);
    }
    i2c_ll_clear_intr_mask(I2C_DEV, I2C_INTR_MST_RXFIFO_WM);
}

// Interrupt handler for I²C driver.
void esp_i2c_isr() {
    for (int i2c_num = 0; i2c_num < I2C_COUNT; i2c_num++) {
        uint32_t st;
        i2c_ll_get_intr_mask(I2C_DEV, &st);
        if (st & I2C_INTR_MST_TXFIFO_WM) {
            logk_from_isr(LOG_DEBUG, "I2C_INTR_MST_TXFIFO_WM");
            i2c_isr_txfifo(i2c_num);
        }
        if (st & I2C_INTR_MST_RXFIFO_WM) {
            logk_from_isr(LOG_DEBUG, "I2C_INTR_MST_RXFIFO_WM");
            i2c_isr_rxfifo(i2c_num);
        }
        if (st & I2C_LL_INTR_NACK) {
            logk_from_isr(LOG_DEBUG, "I2C_LL_INTR_NACK");
        }
        if (st & I2C_LL_INTR_TIMEOUT) {
            logk_from_isr(LOG_DEBUG, "I2C_LL_INTR_TIMEOUT");
        }
        if (st & I2C_LL_INTR_MST_COMPLETE) {
            logk_from_isr(LOG_DEBUG, "I2C_LL_INTR_MST_COMPLETE");
            i2c_isr_trans_done(i2c_num);
        }
        if (st & I2C_LL_INTR_ARBITRATION) {
            logk_from_isr(LOG_DEBUG, "I2C_LL_INTR_ARBITRATION");
        }
        if (st & I2C_LL_INTR_END_DETECT) {
            logk_from_isr(LOG_DEBUG, "I2C_LL_INTR_END_DETECT");
        }
        if (st & I2C_LL_INTR_ST_TO) {
            logk_from_isr(LOG_DEBUG, "I2C_LL_INTR_ST_TO");
        }
    }
}



// Initialises I²C peripheral i2c_num in slave mode with SDA pin `sda_pin`, SCL pin `scl_pin` and clock speed/bitrate
// bitrate. When initialised as an I²C master, the modes of the SDA and SCL pins are changed automatically. This
// function may be used again to change the settings on an initialised I²C peripheral in master mode.
void i2c_master_init(badge_err_t *ec, int i2c_num, int sda_pin, int scl_pin, int32_t bitrate) {
    // Bounds check.
    if (i2c_num < 0 || i2c_num >= i2c_count() || sda_pin < 0 || sda_pin >= io_count() || scl_pin < 0 ||
        scl_pin >= io_count()) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_RANGE);
        return;
    }
    gpio_hal_context_t hal = {&GPIO};

    i2c_ctx[i2c_num].sda_pin   = sda_pin;
    i2c_ctx[i2c_num].scl_pin   = scl_pin;
    i2c_ctx[i2c_num].is_master = true;
    i2c_ctx[i2c_num].enabled   = true;

    // Configure pins.
    gpio_ll_set_level(&GPIO, sda_pin, true);
    io_pull(NULL, sda_pin, IO_PULL_NONE);
    gpio_ll_od_enable(&GPIO, sda_pin);
    gpio_hal_func_sel(&hal, scl_pin, PIN_FUNC_GPIO);
    esp_rom_gpio_connect_out_signal(sda_pin, i2c_sigtab[i2c_num].sda_out_sig, false, false);
    esp_rom_gpio_connect_in_signal(sda_pin, i2c_sigtab[i2c_num].sda_in_sig, false);

    gpio_ll_set_level(&GPIO, scl_pin, true);
    io_pull(NULL, scl_pin, IO_PULL_NONE);
    gpio_ll_od_enable(&GPIO, scl_pin);
    gpio_hal_func_sel(&hal, scl_pin, PIN_FUNC_GPIO);
    esp_rom_gpio_connect_out_signal(scl_pin, i2c_sigtab[i2c_num].scl_out_sig, false, false);
    esp_rom_gpio_connect_in_signal(scl_pin, i2c_sigtab[i2c_num].scl_in_sig, false);

    // Configure clock.
    i2c_ll_enable_bus_clock(i2c_num, true);
    i2c_ll_reset_register(i2c_num);
    i2c_ll_set_source_clk(I2C_DEV, I2C_CLK_SRC_XTAL);
    i2c_ll_enable_controller_clock(I2C_DEV, true);

    // Configure hardware.
    i2c_ll_master_init(I2C_DEV);
    i2c_ll_set_data_mode(I2C_DEV, I2C_DATA_MODE_MSB_FIRST, I2C_DATA_MODE_MSB_FIRST);
    i2c_ll_set_txfifo_empty_thr(I2C_DEV, 3);
    i2c_ll_set_rxfifo_full_thr(I2C_DEV, 13);
    i2c_ll_txfifo_rst(I2C_DEV);
    i2c_ll_rxfifo_rst(I2C_DEV);
    i2c_ll_update(I2C_DEV);

    // Set timing.
    i2c_hal_clk_config_t clk_cal = {0};
    i2c_ll_master_cal_bus_clk(XTAL_CLK_FREQ, bitrate, &clk_cal);
    i2c_ll_master_set_bus_timing(I2C_DEV, &clk_cal);
    i2c_ll_master_set_fractional_divider(I2C_DEV, 0, 0);
    i2c_ll_update(I2C_DEV);
    i2c_ll_disable_intr_mask(I2C_DEV, -1);

    badge_err_set_ok(ec);
}

// De-initialises I²C peripheral i2c_num in master mode.
void i2c_master_deinit(badge_err_t *ec, int i2c_num) {
    // Bounds check.
    if (i2c_num < 0 || i2c_num >= i2c_count()) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_RANGE);
        return;
    }
    if (!i2c_ctx[i2c_num].is_master || !i2c_ctx[i2c_num].enabled) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_STATE);
        return;
    }
    i2c_ctx[i2c_num].enabled = false;

    // Disable interrupts.
    i2c_ll_disable_intr_mask(I2C_DEV, I2C_INTR_MST_TXFIFO_WM | I2C_INTR_MST_RXFIFO_WM);

    // Disable hardware.
    i2c_ll_enable_controller_clock(I2C_DEV, false);
    i2c_ll_enable_bus_clock(i2c_num, false);

    // Reset pins.
    esp_rom_gpio_connect_out_signal(i2c_ctx[i2c_num].scl_pin, SIG_GPIO_OUT_IDX, false, false);
    gpio_ll_od_disable(&GPIO, i2c_ctx[i2c_num].scl_pin);
    gpio_ll_set_level(&GPIO, i2c_ctx[i2c_num].scl_pin, false);

    esp_rom_gpio_connect_out_signal(i2c_ctx[i2c_num].sda_pin, SIG_GPIO_OUT_IDX, false, false);
    gpio_ll_od_disable(&GPIO, i2c_ctx[i2c_num].sda_pin);
    gpio_ll_set_level(&GPIO, i2c_ctx[i2c_num].sda_pin, false);

    badge_err_set_ok(ec);
}

// Performs an I²C transaction synchronously.
static size_t i2c_sync_trans(badge_err_t *ec, int i2c_num, i2c_fsm_cmd_t *cmd, size_t cmd_len) {
    i2c_ll_txfifo_rst(I2C_DEV);
    i2c_ll_rxfifo_rst(I2C_DEV);

    // Reset I²C state machine.
    atomic_store(&i2c_ctx[i2c_num].busy, true);
    i2c_ctx[i2c_num].trans_bytes = 0;
    i2c_ctx[i2c_num].cur_cmd     = 0;
    i2c_ctx[i2c_num].cur_len     = 0;
    i2c_ctx[i2c_num].txd_len     = 0;
    i2c_ctx[i2c_num].txd_cmd     = 0;
    i2c_ctx[i2c_num].rxd_len     = 0;
    i2c_ctx[i2c_num].rxd_cmd     = 0;
    i2c_ctx[i2c_num].cmd         = cmd;
    i2c_ctx[i2c_num].cmd_len     = cmd_len;

    // Reset I²C FSM.
    i2c_ll_txfifo_rst(I2C_DEV);
    i2c_ll_rxfifo_rst(I2C_DEV);
    i2c_ll_master_fsm_rst(I2C_DEV);
    // i2c_ll_master_clr_bus(I2C_DEV, I2C_LL_RESET_SLV_SCL_PULSE_NUM_DEFAULT);

    // Start transaction.
    i2c_queue_cmds(i2c_num);
    i2c_isr_txfifo(i2c_num);
    i2c_ll_clear_intr_mask(I2C_DEV, -1);
    i2c_ll_enable_intr_mask(I2C_DEV, I2C_LL_MASTER_EVENT_INTR | I2C_INTR_MST_TXFIFO_WM | I2C_INTR_MST_RXFIFO_WM);
    i2c_ll_update(I2C_DEV);
    logkf(LOG_DEBUG, "SR: %{u32;x}", I2C0.sr.val);
    i2c_ll_master_trans_start(I2C_DEV);
    uint32_t sr = I2C0.sr.val;
    asm("");
    logkf(LOG_DEBUG, "SR: %{u32;x}", sr);
    timestamp_us_t timeout = time_us() + 100000;
    while (atomic_load(&i2c_ctx[i2c_num].busy) && time_us() < timeout) {
        if (time_us() > timeout) {
            badge_err_set(ec, ELOC_I2C, ECAUSE_TIMEOUT);
            return i2c_ctx[i2c_num].trans_bytes;
        }
        sched_yield();
    }
    logkf(LOG_DEBUG, "SR: %{u32;x}", I2C0.sr.val);

    badge_err_set_ok(ec);
    return i2c_ctx[i2c_num].trans_bytes;
}

// Reads len bytes into buffer buf from I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged read bytes.
size_t i2c_master_read_from(badge_err_t *ec, int i2c_num, int slave_id, void *raw_ptr, size_t len) {
    // Bounds check.
    if (i2c_num < 0 || i2c_num >= i2c_count() || !i2c_is_valid_addr(slave_id)) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_RANGE);
        return 0;
    }

    i2c_fsm_addr_t addr      = i2c_fmt_addr(slave_id, I2C_MASTER_WRITE);
    i2c_fsm_cmd_t  i2c_ops[] = {
        {I2C_LL_CMD_RESTART, 0, 0, 0, 0, NULL},
        {I2C_LL_CMD_WRITE, true, I2C_ACK, false, 1 + addr.addr_10bit, addr.addr_buf},
        {I2C_LL_CMD_READ, false, false, I2C_ACK, len, raw_ptr},
        {I2C_LL_CMD_STOP, 0, 0, 0, 0, NULL},
    };

    return i2c_sync_trans(ec, i2c_num, i2c_ops, 4);
}

// Writes len bytes from buffer buf to I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged written bytes.
size_t i2c_master_write_to(badge_err_t *ec, int i2c_num, int slave_id, void const *raw_ptr, size_t len) {
    // Bounds check.
    if (i2c_num < 0 || i2c_num >= i2c_count() || !i2c_is_valid_addr(slave_id)) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_RANGE);
        return 0;
    }

    i2c_fsm_addr_t addr      = i2c_fmt_addr(slave_id, I2C_MASTER_WRITE);
    i2c_fsm_cmd_t  i2c_ops[] = {
        {I2C_LL_CMD_RESTART, 0, 0, 0, 0, NULL},
        {I2C_LL_CMD_WRITE, true, I2C_ACK, false, 1 + addr.addr_10bit, addr.addr_buf},
        {I2C_LL_CMD_WRITE, true, I2C_ACK, false, len, (void *)raw_ptr},
        {I2C_LL_CMD_STOP, 0, 0, 0, 0, NULL},
    };

    return i2c_sync_trans(ec, i2c_num, i2c_ops, 4);
}
