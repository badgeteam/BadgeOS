
// SPDX-License-Identifier: MIT

#pragma once

#include "soc/interrupts.h"

#include <stdint.h>

// Interrupt map register.
typedef union {
    struct {
        // Mapped interrupt signal.
        uint32_t map : 6;
        // Reserved.
        uint32_t     : 26;
    };
    // Packed value.
    uint32_t val;
} intmtx_map_t;

// Clock gate register.
typedef union {
    struct {
        // Clock enabled.
        uint32_t clk_en : 1;
        // Reserved.
        uint32_t        : 31;
    };
    // Packed value.
    uint32_t val;
} intmtx_clk_t;

// Interrupt matrix.
typedef struct {
    // Interrupt map.
    intmtx_map_t volatile map[ETS_MAX_INTR_SOURCE];
    // Interrupts pending.
    uint32_t volatile pending[(ETS_MAX_INTR_SOURCE + 31) / 32];
    // Clock gate register.
    intmtx_clk_t volatile clock;
} intmtx_t;