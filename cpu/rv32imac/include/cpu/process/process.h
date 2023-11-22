
// SPDX-License-Identifier: MIT

#pragma once

#include "process/types.h"

// Generate RISC-V PMP configurations cache for a given process.
// Returns `true` if the memory map can be represented with the configured amount of PMPs.
bool proc_riscv_pmp_gen(proc_memmap_t *memmap);
// Swap in RISC-V PMP configurations for a given process.
void proc_riscv_pmp_swap(proc_memmap_t const *memmap);
