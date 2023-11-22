
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/riscv_pmp.h"
#include "port/hardware_allocation.h"

// RISC-V PMP cache.
typedef struct {
    // PMP configurations.
    riscv_pmpcfg_t pmpcfg[PROC_RISCV_PMP_COUNT];
    // PMP addresses.
    size_t         pmpaddr[PROC_RISCV_PMP_COUNT];
} proc_riscv_pmp_t;
