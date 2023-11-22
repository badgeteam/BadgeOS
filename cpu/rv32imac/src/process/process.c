
// SPDX-License-Identifier: MIT

#include "process/process.h"

#include "cpu/process/process.h"
#include "log.h"
#include "port/hardware_allocation.h"



// Generate RISC-V PMP configurations cache for a given process.
// Returns `true` if the memory map can be represented with the configured amount of PMPs.
bool proc_riscv_pmp_gen(proc_memmap_t *memmap) {
    // TODO.
    (void)memmap;
    logkf(LOG_DEBUG, "Memory map: %{size;d} regions:");
    for (size_t i = 0; i < memmap->regions_len; i++) {
        logkf(
            LOG_DEBUG,
            "%{size;d}: 0x%{size;x} - 0x%{size;x} r%{c}%{c}",
            i,
            memmap->regions[i].base,
            memmap->regions[i].base + memmap->regions[i].size,
            memmap->regions[i].write ? 'w' : '-',
            memmap->regions[i].exec ? 'x' : '-'
        );
    }
    return true;
}

// Swap in RISC-V PMP configurations for a given process.
void proc_riscv_pmp_swap(proc_memmap_t const *memmap) {
    // TODO.
    (void)memmap;
}
