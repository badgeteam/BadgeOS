
// SPDX-License-Identifier: MIT

#include "hal/i2c.h"

#include "hal/gpio.h"
#include "port/clkconfig.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/i2c_struct.h"
#include "soc/io_mux_struct.h"
#include "soc/pmu_struct.h"

// I2C command register value.
typedef union {
    struct {
        // Number of bytes for reading or writing.
        uint32_t byte_num     : 8;
        // Enable checking of ACK bit.
        uint32_t ack_check_en : 1;
        // Expected value of ACK bit.
        uint32_t ack_exp      : 1;
        // Sent value of ACK bit.
        uint32_t ack_value    : 1;
        // I2C command type.
        uint32_t op_code      : 3;
    };
    uint32_t value;
} i2c_comd_val_t;

#define I2C_OPC_WRITE  1
#define I2C_OPC_STOP   2
#define I2C_OPC_READ   3
#define I2C_OPC_END    4
#define I2C_OPC_RSTART 6

#define I2C_ACK  0
#define I2C_NACK 1



// Load commands into the command buffer.
static void i2c_master_load_comd(i2c_comd_val_t *comd, size_t count) {
    assert_dev_drop(count > 0 && count <= 8);
    for (size_t i = 0; i < count; i++) {
        I2C0.command[i] = (i2c_comd_reg_t){
            .command_done = false,
            .command      = comd[i].value,
        };
    }
}

// Add an I2C address to the TX queue.
// Returns true for 10-bit addresses, false for 7-bit addresses.
static bool i2c_master_queue_addr(int slave_id, bool read_bit) {
    assert_dev_drop(slave_id >= 0 && slave_id <= 1023);
    if (slave_id > 127) {
        I2C0.data.val = 0b11110000 | ((slave_id >> 7) & 0b0110) | read_bit;
        I2C0.data.val = slave_id;
        return true;
    } else {
        I2C0.data.val = (slave_id << 1) | read_bit;
        return false;
    }
}

// Clear I2C FIFOs.
static void i2c_clear_fifo(bool clear_rxfifo, bool clear_txfifo) {
    i2c_fifo_conf_reg_t tmp = I2C0.fifo_conf;
    tmp.tx_fifo_rst         = clear_txfifo;
    tmp.rx_fifo_rst         = clear_rxfifo;
    I2C0.fifo_conf          = tmp;
    tmp.tx_fifo_rst         = false;
    tmp.rx_fifo_rst         = false;
    I2C0.fifo_conf          = tmp;
}

// Initialises I²C peripheral i2c_num in slave mode with SDA pin `sda_pin`, SCL pin `scl_pin` and clock speed/bitrate
// bitrate. When initialised as an I²C master, the modes of the SDA and SCL pins are changed automatically. This
// function may be used again to change the settings on an initialised I²C peripheral in master mode.
void i2c_master_init(badge_err_t *ec, int i2c_num, int sda_pin, int scl_pin, int32_t bitrate) {
    // Bounds check.
    if (i2c_num != 0 || sda_pin < 0 || sda_pin >= io_count() || scl_pin < 0 || scl_pin >= io_count()) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_RANGE);
        return;
    }

    // Pin availability check.
    if (io_is_peripheral(ec, sda_pin)) {
        logkf(LOG_ERROR, "SDA pin (%{d}) already in use", sda_pin);
        return;
    } else if (io_is_peripheral(ec, scl_pin)) {
        logkf(LOG_ERROR, "SCL pin (%{d}) already in use", scl_pin);
        return;
    }

    // Clock configuration.
    clkconfig_i2c0(bitrate * 10, true, false);

    // I2C master configuration.
    I2C0.ctr = (i2c_ctr_reg_t){
        .sda_force_out = true,
        .scl_force_out = true,
        .ms_mode       = true,
    };

    // Clear FIFOs.
    I2C0.fifo_conf = (i2c_fifo_conf_reg_t){
        .tx_fifo_rst = true,
        .rx_fifo_rst = true,
    };
    I2C0.fifo_conf = (i2c_fifo_conf_reg_t){
        .rxfifo_wm_thrhd  = 0x0b,
        .txfifo_wm_thrhd  = 0x04,
        .nonfifo_en       = false,
        .fifo_addr_cfg_en = false,
        .tx_fifo_rst      = false,
        .rx_fifo_rst      = false,
        .fifo_prt_en      = true,
    };

    // Timeout configuration.
    I2C0.to = (i2c_to_reg_t){
        .time_out_value = 16,
        .time_out_en    = true,
    };
    I2C0.scl_st_time_out.val      = 0x10;
    I2C0.scl_main_st_time_out.val = 0x10;

    I2C0.sda_hold.sda_hold_time     = 30;
    I2C0.sda_sample.sda_sample_time = 30;

    I2C0.scl_low_period.scl_low_period          = 50;
    I2C0.scl_high_period.scl_high_period        = 25;
    I2C0.scl_high_period.scl_wait_high_period   = 25;
    I2C0.scl_rstart_setup.scl_rstart_setup_time = 100;
    I2C0.scl_start_hold.scl_start_hold_time     = 100;
    I2C0.scl_stop_setup.scl_stop_setup_time     = 100;
    I2C0.scl_stop_hold.scl_stop_hold_time       = 100;

    I2C0.ctr.conf_upgate = true;

    // Make GPIO open-drain.
    GPIO.pin[sda_pin]    = (gpio_pin_reg_t){.pad_driver = true};
    GPIO.pin[scl_pin]    = (gpio_pin_reg_t){.pad_driver = true};
    IO_MUX.gpio[sda_pin] = (io_mux_gpio_t){.mcu_sel = 1, .fun_ie = true, .mcu_ie = true};
    IO_MUX.gpio[scl_pin] = (io_mux_gpio_t){.mcu_sel = 1, .fun_ie = true, .mcu_ie = true};

    // GPIO matrix configuration.
    GPIO.func_out_sel_cfg[sda_pin] = (gpio_func_out_sel_cfg_reg_t){
        .oen_inv_sel = false,
        .oen_sel     = false,
        .out_inv_sel = false,
        .out_sel     = I2CEXT0_SDA_OUT_IDX,
    };
    GPIO.func_out_sel_cfg[scl_pin] = (gpio_func_out_sel_cfg_reg_t){
        .oen_inv_sel = false,
        .oen_sel     = false,
        .out_inv_sel = false,
        .out_sel     = I2CEXT0_SCL_OUT_IDX,
    };
    GPIO.func_in_sel_cfg[I2CEXT0_SDA_IN_IDX] = (gpio_func_in_sel_cfg_reg_t){
        .in_sel     = sda_pin,
        .in_inv_sel = false,
        .sig_in_sel = true,
    };
    GPIO.func_in_sel_cfg[I2CEXT0_SCL_IN_IDX] = (gpio_func_in_sel_cfg_reg_t){
        .in_sel     = scl_pin,
        .in_inv_sel = false,
        .sig_in_sel = true,
    };
}

// De-initialises I²C peripheral i2c_num in master mode.
void i2c_master_deinit(badge_err_t *ec, int i2c_num) {
    (void)ec;
    (void)i2c_num;
}

// Reads len bytes into buffer buf from I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged read bytes.
size_t i2c_master_read_from(badge_err_t *ec, int i2c_num, int slave_id, void *raw_ptr, size_t len) {
    uint8_t *buf = (uint8_t *)raw_ptr;

    // Bounds check.
    if (i2c_num != 0 || len > 255 || slave_id < 0 || slave_id > 1023) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_RANGE);
        return 0;
    }

    // Put address in the FIFO.
    i2c_clear_fifo(true, true);
    bool addr_10bit = i2c_master_queue_addr(slave_id, true);

    // Set commands.
    i2c_comd_val_t cmd[] = {
        // Start condition.
        {
            .op_code = I2C_OPC_RSTART,
        },
        // Address.
        {
            .op_code      = I2C_OPC_WRITE,
            .ack_check_en = true,
            .ack_exp      = I2C_ACK,
            .ack_value    = I2C_ACK,
            .byte_num     = 1 + addr_10bit,
        },
        // Read.
        {
            .op_code      = I2C_OPC_READ,
            .ack_check_en = false,
            .ack_value    = I2C_ACK,
            .byte_num     = len,
        },
        // Stop condition.
        {
            .op_code = I2C_OPC_STOP,
        },
        // End.
        {
            .op_code = I2C_OPC_END,
        },
    };
    i2c_master_load_comd(cmd, sizeof(cmd) / sizeof(i2c_comd_val_t));

    // Start the transaction.
    I2C0.ctr.conf_upgate = true;
    I2C0.ctr.trans_start = true;

    // Wait for transaction to finish.
    timestamp_us_t to = time_us() + 10000;
    while (I2C0.sr.bus_busy && time_us() < to)
        ;

    for (size_t i = 0; i < len; i++) {
        buf[i] = I2C0.data.fifo_rdata;
    }

    return 0;
}

// Writes len bytes from buffer buf to I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged written bytes.
size_t i2c_master_write_to(badge_err_t *ec, int i2c_num, int slave_id, void const *raw_ptr, size_t len) {
    uint8_t const *buf = (uint8_t const *)raw_ptr;

    // Bounds check.
    if (i2c_num != 0 || len > 255) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_RANGE);
        return 0;
    }

    // Put address in the FIFO.
    i2c_clear_fifo(true, true);
    bool addr_10bit = i2c_master_queue_addr(slave_id, false);

    // Set commands.
    i2c_comd_val_t cmd[] = {
        // Start condition.
        {
            .op_code = I2C_OPC_RSTART,
        },
        // Address.
        {
            .op_code      = I2C_OPC_WRITE,
            .ack_check_en = true,
            .ack_exp      = I2C_ACK,
            .ack_value    = I2C_ACK,
            .byte_num     = 1 + addr_10bit,
        },
        // Write.
        {
            .op_code      = I2C_OPC_WRITE,
            .ack_check_en = false,
            .ack_value    = I2C_ACK,
            .byte_num     = len,
        },
        // Stop condition.
        {
            .op_code = I2C_OPC_STOP,
        },
        // End.
        {
            .op_code = I2C_OPC_END,
        },
    };
    i2c_master_load_comd(cmd, sizeof(cmd) / sizeof(i2c_comd_val_t));

    // Put all write data.
    for (size_t i = 0; i < len; i++) {
        // Wait for FIFO space to become available.
        i2c_fifo_st_reg_t fifo_st;
        do {
            fifo_st = I2C0.fifo_st;
        } while (fifo_st.txfifo_raddr == (fifo_st.txfifo_waddr + 1) % 31);

        // Write the byte into to FIFO.
        I2C0.data.fifo_rdata = buf[i];
    }

    // Start the transaction.
    I2C0.ctr.conf_upgate = true;
    I2C0.ctr.trans_start = true;

    // Wait for transaction to finish.
    while (I2C0.sr.bus_busy)
        ;

    return 0;
}
