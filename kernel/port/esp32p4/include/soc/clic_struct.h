
// SPDX-License-Identifier: MIT

#pragma once

typedef union {
    struct {
        uint32_t nvbits : 1;
        uint32_t nlbits : 4;
        uint32_t nmbits : 2;
        uint32_t        : 25;
    };
    // Packed value.
    uint32_t val;
} clic_dev_int_config_reg_t;

typedef union {
    struct {
        uint32_t num_int : 13;
        uint32_t version : 8;
        uint32_t ctlbits : 4;
        uint32_t         : 7;
    };
    // Packed value.
    uint32_t val;
} clic_dev_int_info_reg_t;

typedef union {
    struct {
        uint32_t thresh : 8;
        uint32_t        : 24;
    };
    // Packed value.
    uint32_t val;
} clic_dev_int_thresh_reg_t;

typedef struct {
    clic_dev_int_config_reg_t volatile int_config;
    clic_dev_int_info_reg_t volatile int_info;
    clic_dev_int_thresh_reg_t volatile int_thresh;
} clic_dev_t;



typedef union {
    struct {
        uint32_t pending   : 1;
        uint32_t           : 7;
        uint32_t enable    : 1;
        uint32_t           : 7;
        uint32_t attr_shv  : 1;
        uint32_t attr_trig : 1;
        uint32_t attr_mode : 1;
        uint32_t           : 13;
    };
    // Packed value.
    uint32_t val;
} clic_int_ctl_reg_t;

typedef union {
    struct {
        clic_int_ctl_reg_t volatile intirq_ctl[16];
        clic_int_ctl_reg_t volatile extirq_ctl[32];
    };
    clic_int_ctl_reg_t volatile irq_ctl[48];
} clic_ctl_dev_t;
