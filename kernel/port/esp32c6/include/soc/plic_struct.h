
// SPDX-License-Identifier: MIT

#pragma once

#include "soc/soc.h"

typedef struct {
    // Interrupt enable.
    uint32_t volatile int_en;
    // Interrupt type.
    uint32_t volatile int_type;
    // Interrupt clear.
    uint32_t volatile int_clear;
    // EMIP status register.
    uint32_t volatile int_pending;

    // Interrupt priorities.
    uint32_t volatile int_pri[32];
} esp_plic_t;

extern esp_plic_t PLIC_MX;
extern esp_plic_t PLIC_UX;
