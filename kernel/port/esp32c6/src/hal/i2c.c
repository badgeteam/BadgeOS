
// SPDX-License-Identifier: MIT

#include "hal/i2c.h"

#include "hal/gpio.h"
#include "interrupt.h"
#include "malloc.h"
#include "port/clkconfig.h"
#include "scheduler/scheduler.h"
#include "soc/ext_irq.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/i2c_struct.h"
#include "soc/io_mux_struct.h"
#include "soc/pcr_struct.h"
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

// Pending transaction entry.
typedef struct {
    dlist_node_t node;
    i2c_trans_t  trans;
} pending_t;

// I²C driver state.
typedef struct {
    // I²C peripheral.
    int        i2c_num;
    // I²C device.
    i2c_dev_t *dev;
    // FIFO size.
    size_t     fifo_size;

    // Pending transaction mutex.
    mutex_t mtx;
    // Pending asynchronous I²C transactions.
    dlist_t pending;

    // Current I²C transaction.
    i2c_trans_t *trans;
    // Next I²C command to run.
    i2c_cmd_t   *next_cmd;
    // Next I²C command to process TX data for.
    i2c_cmd_t   *next_txd;
    // Next I²C command to process RX data for.
    i2c_cmd_t   *next_rxd;

    // I²C is busy.
    atomic_flag busy;
    // I²C ISR busy.
    atomic_flag isr;
    // I²C is being driven by async handler.
    bool        is_async;
} i2c_driver_t;

// I²C driver state.
static i2c_driver_t drivers[i2c_count()] = {{
    0,
    &I2C0,
    31,
    MUTEX_T_INIT,
    DLIST_EMPTY,
    NULL,
    NULL,
    NULL,
    NULL,
    ATOMIC_FLAG_INIT,
    ATOMIC_FLAG_INIT,
    false,
}};



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

// Queue as much of the commands as possible.
// Returns whether there was anything to queue.
static bool i2c_driver_cmd(i2c_driver_t *driver) {
    size_t         i;
    i2c_comd_val_t comd[8];

    for (i = 0; i < 8 && driver->next_cmd; i++) {
        // Translate abstract commands into hardware commands.
        switch (driver->next_cmd->type) {
            default: __builtin_unreachable();
            case I2C_CMD_START:
                comd[i] = (i2c_comd_val_t){
                    .op_code = I2C_OPC_RSTART,
                };
                break;
            case I2C_CMD_STOP:
                comd[i] = (i2c_comd_val_t){
                    .op_code = I2C_OPC_STOP,
                };
                break;
            case I2C_CMD_ADDR:
                comd[i] = (i2c_comd_val_t){
                    .op_code      = I2C_OPC_WRITE,
                    .ack_check_en = true,
                    .ack_exp      = I2C_ACK,
                    .byte_num     = 1 + driver->next_cmd->addr_10bit,
                };
                break;
            case I2C_CMD_WRITE:
                comd[i] = (i2c_comd_val_t){
                    .op_code      = I2C_OPC_WRITE,
                    .ack_check_en = true,
                    .ack_exp      = I2C_ACK,
                    .byte_num     = driver->next_cmd->length,
                };
                break;
            case I2C_CMD_READ:
                comd[i] = (i2c_comd_val_t){
                    .op_code      = I2C_OPC_READ,
                    .ack_check_en = false,
                    .ack_value    = I2C_ACK,
                    .byte_num     = driver->next_cmd->length,
                };
                break;
        }
        driver->next_cmd = (i2c_cmd_t *)driver->next_cmd->node.next;
    }

    // Load the commands into the peripheral.
    for (size_t x = 0; x < i; x++) {
        I2C0.command[x] = (i2c_comd_reg_t){
            .command_done = false,
            .command      = comd[x].value,
        };
    }
    if (i < 8) {
        I2C0.command[i] = (i2c_comd_reg_t){
            .command_done = false,
            .command      = ((i2c_comd_val_t){.op_code = I2C_OPC_END}).value,
        };
    }

    return i;
}

// Queue as much TX data as possible.
static bool i2c_driver_txdata(i2c_driver_t *driver) {
    bool added = false;
    while (driver->next_txd && driver->dev->sr.txfifo_cnt < driver->fifo_size) {
        switch (driver->next_txd->type) {
            default: driver->next_txd = (i2c_cmd_t *)driver->next_txd->node.next; break;
            case I2C_CMD_WRITE:
                if (driver->next_txd->length > I2C_SMALL_WRITE_SIZE) {
                    // Large data byte.
                    driver->dev->data.val = driver->next_txd->data[driver->next_txd->index++];
                } else {
                    // Small data byte.
                    driver->dev->data.val = driver->next_txd->small_data[driver->next_txd->index++];
                }
                if (driver->next_txd->index >= driver->next_txd->length) {
                    // Write finished.
                    driver->next_txd = (i2c_cmd_t *)driver->next_txd->node.next;
                }
                added = true;
                break;
            case I2C_CMD_ADDR:
                if (driver->next_txd->addr_10bit && driver->next_txd->index == 0) {
                    // First byte of 10-bit address.
                    driver->dev->data.val = 0xf0 | ((driver->next_txd->addr >> 7) & 0x06) | driver->next_txd->read_bit;
                    driver->next_txd->index = 1;
                } else if (driver->next_txd->addr_10bit) {
                    // Second byte of 10-bit address.
                    driver->dev->data.val = driver->next_txd->addr;
                    driver->next_txd      = (i2c_cmd_t *)driver->next_txd->node.next;
                } else {
                    // 7-bit address.
                    driver->dev->data.val = (driver->next_txd->addr << 1) | driver->next_txd->read_bit;
                    driver->next_txd      = (i2c_cmd_t *)driver->next_txd->node.next;
                }
                added = true;
                break;
        }
    }
    return added;
}

// Receive as much RX data as possible.
static bool i2c_driver_rxdata(i2c_driver_t *driver) {
    return false;
    // while (driver->dev->sr.rxfifo_cnt) {
    //     if (!driver->next_rxd) {
    //         return false;
    //     } else if (driver->next_rxd->type != I2C_CMD_READ) {
    //         driver->next_rxd = (i2c_cmd_t *)driver->next_rxd->node.next;
    //     } else {
    //         driver->next_rxd->data[driver->next_rxd->index++] = driver->dev->data.val;
    //         if (driver->next_rxd->index >= driver->next_rxd->length) {
    //             driver->next_rxd = (i2c_cmd_t *)driver->next_rxd->node.next;
    //         }
    //     }
    // }
    // return true;
}


// Try to start an I²C transaction.
static bool i2c_driver_begin(i2c_driver_t *driver, i2c_trans_t *trans) {
    // Try to claim the I²C.
    if (atomic_flag_test_and_set(&driver->busy)) {
        return false;
    }
    // Set the ISR busy flag.
    atomic_flag_test_and_set(&driver->isr);

    // Start the transaction.
    driver->trans    = trans;
    driver->next_cmd = (i2c_cmd_t *)trans->list.head;
    driver->next_txd = (i2c_cmd_t *)trans->list.head;
    driver->next_rxd = (i2c_cmd_t *)trans->list.head;
    i2c_clear_fifo(true, true);
    if (i2c_driver_cmd(driver)) {
        // Commands queued, begin the transaction.
        i2c_driver_txdata(driver);
        driver->dev->int_ena.txfifo_wm_int_ena = true;
        // driver->dev->int_ena.rxfifo_wm_int_ena = true;
        driver->dev->ctr.fsm_rst               = true;
        driver->dev->ctr.fsm_rst               = false;
        driver->dev->ctr.conf_upgate           = true;
        driver->dev->ctr.trans_start           = true;
    }

    return true;
}

// The I²C ISR.
static void i2c_driver_isr(i2c_driver_t *driver) {
    i2c_int_status_reg_t irq = driver->dev->int_status;
    i2c_dev_t           *dev = driver->dev;

    // RXFIFO full IRQ.
    if (irq.rxfifo_wm_int_st) {
        // Read RXFIFO.
        if (!i2c_driver_rxdata(driver)) {
            I2C0.int_ena.rxfifo_wm_int_ena = false;
        }
        // Clear interrupt flag.
        dev->int_clr.rxfifo_wm_int_clr = true;
        dev->int_clr.val               = 0;
    }

    // TXFIFO empty IRQ.
    if (irq.txfifo_wm_int_st) {
        // Write TXFIFO.
        if (!i2c_driver_txdata(driver)) {
            I2C0.int_ena.txfifo_wm_int_ena = false;
        }
        // Clear interrupt flag.
        dev->int_clr.txfifo_wm_int_clr = true;
        dev->int_clr.val               = 0;
    }

    // Transmission finished IRQ.
    if (irq.trans_complete_int_st) {
        // Clear interrupt flag.
        dev->int_clr.trans_complete_int_clr = true;
        dev->int_clr.val                    = 0;
        // Try to queue more I²C transaction.
        if (i2c_driver_cmd(driver)) {
            // Commands queued, resume the transaction.
            driver->dev->ctr.fsm_rst = true;
            driver->dev->ctr.fsm_rst = false;
            dev->ctr.trans_start     = true;
        } else {
            // Read remaining RXFIFO.
            i2c_driver_rxdata(driver);
            I2C0.int_ena.txfifo_wm_int_ena = false;
            I2C0.int_ena.rxfifo_wm_int_ena = false;
            // Nothing more to queue, send finished trigger.
            atomic_flag_clear(&driver->isr);
        }
    }
}

// Asynchrounous I²C driver.
static void i2c_driver_async(i2c_driver_t *driver) {
    // Check for I²C busy.
    // The order of evaluation is critical here; the test/set must not happen is is_async == false.
    if (driver->is_async && !atomic_flag_test_and_set(&driver->isr)) {
        // Release I²C.
        i2c_trans_t *trans = driver->trans;
        driver->is_async   = false;
        atomic_flag_clear(&driver->busy);
        // Clean up transaction.
        if (driver->trans->callback) {
            driver->trans->callback((badge_err_t){}, 0, driver->trans->cookie);
        }
        i2c_trans_destroy(trans);
    }

    // Check for pending async.
    mutex_acquire(NULL, &driver->mtx, TIMESTAMP_US_MAX);
    pending_t *head = (pending_t *)driver->pending.head;
    if (head && i2c_driver_begin(driver, &head->trans)) {
        // I²C successfully started; remove from the queue.
        driver->is_async = true;
        dlist_pop_front(&driver->pending);
        free(head);
    }
    mutex_release(NULL, &driver->mtx);
}

// Synchronous I²C driver.
static size_t i2c_driver_sync(i2c_driver_t *driver, badge_err_t *ec, i2c_trans_t *trans) {
    // Wait until I²C is available.
    while (!i2c_driver_begin(driver, trans)) {
        sched_yield();
    }

    // Wait until I²C is done.
    while (atomic_flag_test_and_set(&driver->isr)) {
        sched_yield();
    }

    // Release I²C.
    atomic_flag_clear(&driver->busy);

    // Clean up transaction.
    if (driver->trans->callback) {
        driver->trans->callback((badge_err_t){}, 0, driver->trans->cookie);
    }
    i2c_trans_destroy(trans);
    badge_err_set_ok(ec);
    return 0;
}


// The I²C ISR.
static void i2c_isr_wrapper() {
    if (I2C0.int_status.val) {
        i2c_driver_isr(&drivers[0]);
    }
}

// Install the I²C ISR.
void port_i2c_install_isr(int channel) {
    irq_ch_set_isr(channel, i2c_isr_wrapper);
    irq_ch_route(EXT_IRQ_I2C_EXT0_INTR, channel);
    irq_ch_enable(channel, true);
}

// Asynchrounous I²C management callback.
void port_i2c_async_cb(int taskno, void *arg) {
    (void)taskno;
    (void)arg;
    for (int i = 0; i < i2c_count(); i++) {
        i2c_driver_async(&drivers[i]);
    }
}



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

    // Make GPIO open-drain.
    GPIO.pin[sda_pin]    = (gpio_pin_reg_t){.pad_driver = true};
    GPIO.pin[scl_pin]    = (gpio_pin_reg_t){.pad_driver = true};
    IO_MUX.gpio[sda_pin] = (io_mux_gpio_t){.mcu_sel = 1, .fun_ie = true, .mcu_ie = true};
    IO_MUX.gpio[scl_pin] = (io_mux_gpio_t){.mcu_sel = 1, .fun_ie = true, .mcu_ie = true};

    // ISR configuration.
    bool mie     = irq_enable(false);
    I2C0.int_ena = (i2c_int_ena_reg_t){
        .rxfifo_wm_int_ena      = true,
        .txfifo_wm_int_ena      = true,
        .trans_complete_int_ena = true,
    };
    I2C0.int_clr.val = -1;
    I2C0.int_clr.val = 0;
    irq_enable(mie);
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
    while (I2C0.sr.bus_busy && time_us() < to);

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
    while (I2C0.sr.bus_busy) continue;

    return 0;
}

// Perform a preconstructed transaction and clean it up afterward.
// Returns how many non-address bytes were exchanged successfully.
size_t i2c_master_run(badge_err_t *ec, int i2c_num, i2c_trans_t *trans) {
    if (i2c_num != 0) {
        badge_err_set(ec, ELOC_I2C, ECAUSE_RANGE);
        return 0;
    }
    return i2c_driver_sync(&drivers[i2c_num], ec, trans);
}
