
// SPDX-License-Identifier: MIT

#include "hal/spi.h"

#include <badge_strings.h>

#include "interrupt.h"
#include "hal/gpio.h"
#include "port/clkconfig.h"
#include "port/hardware_allocation.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/spi_struct.h"
#include "soc/io_mux_struct.h"

static void spi_clear_fifo(bool clear_rxfifo, bool clear_txfifo) {
    GPSPI2.dma_conf = (spi_dma_conf_reg_t){
        .buf_afifo_rst = clear_txfifo,
        .rx_afifo_rst = clear_rxfifo,
    };
    GPSPI2.dma_conf = (spi_dma_conf_reg_t){
        .buf_afifo_rst = false,
        .rx_afifo_rst = false,
    };
}

static void spi_config_apply(void) {
    // Apply the register configurations and wait until it's done.
    GPSPI2.cmd.update = 1;
    while(GPSPI2.cmd.update);
}

static void spi_isr(void) {
    logk(LOG_DEBUG, "SPI ISR");
    if (GPSPI2.dma_int_raw.trans_done) {
        GPSPI2.dma_int_clr.trans_done = 1;
    }
}

static void spi_int_config(bool enable, int channel) {
    // Disable interrupts before changing interrupt settings.
    bool mie = irq_enable(false);

    if (enable) {
        irq_ch_route(EXT_IRQ_GPSPI2_INTR, channel);
        irq_ch_set_isr(channel, spi_isr);
        irq_ch_prio(channel, INT_PRIO_SPI);
    }
    irq_ch_enable(channel, enable);
    GPSPI2.dma_int_ena.trans_done = enable;

    // Re-enable interrupts.
    asm volatile("fence");
    irq_enable(mie);
}

void spi_controller_init(badge_err_t *ec, int spi_num, int sclk_pin, int mosi_pin, int miso_pin, int ss_pin, int32_t bitrate) {
    // Bounds check.
    if (spi_num != 0
    || sclk_pin < 0 || sclk_pin >= io_count()
    || mosi_pin < 0 || mosi_pin >= io_count()
    || miso_pin < 0 || miso_pin >= io_count()
    || ss_pin < 0 || ss_pin >= io_count()
    ) {
        badge_err_set(ec, ELOC_SPI, ECAUSE_RANGE);
        return;
    }

    // Pin availability check.
    if (io_is_peripheral(ec, sclk_pin)) {
        logkf(LOG_ERROR, "SCLK pin (%{d}) already in use", sclk_pin);
        return;
    } else if (io_is_peripheral(ec, mosi_pin)) {
        logkf(LOG_ERROR, "MOSI pin (%{d}) already in use", mosi_pin);
        return;
    } else if (io_is_peripheral(ec, miso_pin)) {
        logkf(LOG_ERROR, "MISO pin (%{d}) already in use", miso_pin);
        return;
    } else if (io_is_peripheral(ec, ss_pin)) {
        logkf(LOG_ERROR, "SS pin (%{d}) already in use", ss_pin);
        return;
    }

    // Clock configuration.
    clkconfig_spi2(bitrate, true, false);

    spi_int_config(true, INT_CHANNEL_SPI);

    spi_config_apply();

    // Set pins to GPIO function and enable input for MISO
    IO_MUX.gpio[sclk_pin] = (io_mux_gpio_t){.mcu_sel = 1,};
    IO_MUX.gpio[miso_pin] = (io_mux_gpio_t){.mcu_sel = 1, .fun_ie = true};
    IO_MUX.gpio[mosi_pin] = (io_mux_gpio_t){.mcu_sel = 1};
    IO_MUX.gpio[ss_pin] = (io_mux_gpio_t){.mcu_sel = 1};

    // GPIO matrix configuration.
    GPIO.func_out_sel_cfg[sclk_pin] = (gpio_func_out_sel_cfg_reg_t){
        .oen_inv_sel = false,
        .oen_sel     = false,
        .out_inv_sel = false,
        .out_sel     = FSPICLK_OUT_IDX,
    };
    GPIO.func_out_sel_cfg[mosi_pin] = (gpio_func_out_sel_cfg_reg_t){
        .oen_inv_sel = false,
        .oen_sel     = false,
        .out_inv_sel = false,
        .out_sel     = FSPID_OUT_IDX,
    };
    GPIO.func_out_sel_cfg[ss_pin] = (gpio_func_out_sel_cfg_reg_t){
        .oen_inv_sel = false,
        .oen_sel     = false,
        .out_inv_sel = false,
        .out_sel     = FSPICS0_OUT_IDX,
    };

    GPIO.func_in_sel_cfg[FSPIQ_IN_IDX] = (gpio_func_in_sel_cfg_reg_t){
        .in_sel     = miso_pin,
        .in_inv_sel = false,
        .sig_in_sel = true,
    };
}

static void spi_controller_transfer_generic(badge_err_t *ec, int spi_num, void *buf, size_t len) {
    const int data_buf_len = sizeof(GPSPI2.data_buf);
    uint32_t words [data_buf_len/sizeof(GPSPI2.data_buf[0])];

    // Bounds check.
    if (spi_num != 0) {
        badge_err_set(ec, ELOC_SPI, ECAUSE_RANGE);
        return;
    }

    while (len > 0) {
        size_t copy_len = (len > data_buf_len) ? data_buf_len : len;

        if (GPSPI2.user.usr_mosi) {
            // Access to SPI data buffer must be in full 32 bit words
            // This assumes optimization for alignment in mem_copy
            mem_copy(words, buf, copy_len);
            mem_copy((void *)GPSPI2.data_buf, words, data_buf_len); // TODO: optimize copy length
        }
        
        // prepare for transfer
        GPSPI2.ms_dlen.ms_data_bitlen = copy_len * 8 - 1;
        spi_clear_fifo(true, true);
        spi_config_apply();

        // Start transfer and wait for completion
        GPSPI2.cmd.usr = 1;
        while(GPSPI2.cmd.usr); // TODO: yield?

        if (GPSPI2.user.usr_miso) {
            // copy back received data
            mem_copy(buf, (void *)GPSPI2.data_buf, copy_len);
        }

        len -= copy_len;
        buf += copy_len;
    }
}

void spi_controller_read(badge_err_t *ec, int spi_num, void *buf, size_t len) {
    GPSPI2.user.usr_mosi = 0;
    GPSPI2.user.usr_miso = 1;

    spi_controller_transfer_generic(ec, spi_num, buf, len);
}

void spi_controller_write(badge_err_t *ec, int spi_num, void const *buf, size_t len) {
    GPSPI2.user.usr_mosi = 1;
    GPSPI2.user.usr_miso = 0;

    spi_controller_transfer_generic(ec, spi_num, (void*) buf, len);
}

void spi_controller_transfer(badge_err_t *ec, int spi_num, void *buf, size_t len, bool fdx) {
    GPSPI2.user.usr_mosi = 1;
    GPSPI2.user.usr_miso = 1;
    GPSPI2.user.doutdin = fdx;

    spi_controller_transfer_generic(ec, spi_num, buf, len);
}
