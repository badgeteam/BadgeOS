
// SPDX-License-Identifier: MIT

#pragma once

typedef union {
    struct {
        // Whether `Smclicshv` is supported.
        uint32_t nvbits : 1;
        // Number of bits used to represent interrupt priority.
        uint32_t nlbits : 4;
        // Number of privilege modes supported.
        // When 0, only M-mode has interrupts.
        uint32_t nmbits : 2;
        uint32_t        : 25;
    };
    // Packed value.
    uint32_t val;
} clic_dev_int_config_reg_t;

typedef union {
    struct {
        // Actual number of interrupt channels.
        uint32_t num_int : 13;
        // CLIC hardware version.
        uint32_t version : 8;
        // Number of bits used to represent interrupt priority.
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
        // Interrupt is pending.
        uint32_t pending   : 1;
        uint32_t           : 7;
        // Interrupt is enabled.
        uint32_t enable    : 1;
        uint32_t           : 7;
        // TODO: What is this?
        uint32_t attr_shv  : 1;
        // Whether the interrupt is edge-triggered.
        uint32_t attr_trig : 1;
        uint32_t           : 4;
        // Which privilege mode receives this interrupt.
        uint32_t attr_mode : 2;
        // Interrupt priority.
        uint32_t ctl       : 8;
    };
    // Packed value.
    uint32_t val;
} clic_int_ctl_reg_t;

typedef union {
    clic_int_ctl_reg_t volatile irq_ctl[48];
} clic_ctl_dev_t;
