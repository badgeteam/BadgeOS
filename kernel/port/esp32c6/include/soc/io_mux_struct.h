
// SPDX-License-Identifier: MIT

#include <stdint.h>

// I2S output clock selection.
typedef union {
    struct {
        uint32_t out1 : 5;
        uint32_t out2 : 5;
        uint32_t out3 : 5;
        uint32_t      : 17;
    };
    uint32_t val;
} io_mux_clk_t;

typedef union {
    struct {
        uint32_t mcu_oe    : 1;
        uint32_t slp_sel   : 1;
        uint32_t mcu_wpd   : 1;
        uint32_t mcu_wpu   : 1;
        uint32_t mcu_ie    : 1;
        uint32_t mcu_drv   : 2;
        uint32_t fun_wpd   : 1;
        uint32_t fun_wpu   : 1;
        uint32_t fun_ie    : 1;
        uint32_t fun_drv   : 2;
        uint32_t mcu_sel   : 3;
        uint32_t filter_en : 1;
    };
    uint32_t val;
} io_mux_gpio_t;

typedef struct io_mux_t {
    volatile io_mux_clk_t  clk;
    volatile io_mux_gpio_t gpio[31];
} io_mux_t;

extern io_mux_t IO_MUX;
