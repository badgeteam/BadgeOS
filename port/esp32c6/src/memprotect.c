
// SPDX-License-Identifier: MIT

#include "memprotect.h"

#include "cpu/riscv_pmp.h"
#include "port/hardware_allocation.h"
#include "port/interrupt.h"

// Initialise memory protection driver.
void memprotect_init() {
    // Initialise PMP driver.
    riscv_pmp_init();

    // Add lower NULLPTR protection range.
    riscv_pmpaddr_write(PMP_ENTRY_NULLPTR_LOW_NAPOT, riscv_pmpaddr_calc_napot(0, PMP_SIZE_NULLPTR));
    riscv_pmpcfg_set(
        PMP_ENTRY_NULLPTR_LOW_NAPOT,
        ((riscv_pmpcfg_t){
            .read            = false,
            .write           = false,
            .exec            = false,
            .addr_match_mode = RISCV_PMPCFG_NAPOT,
            ._reserved       = 0,
            .lock            = true,
        })
    );

    // Add upper NULLPTR protection range.
    riscv_pmpaddr_write(PMP_ENTRY_NULLPTR_HIGH_NAPOT, riscv_pmpaddr_calc_napot(-PMP_SIZE_NULLPTR, PMP_SIZE_NULLPTR));
    riscv_pmpcfg_set(
        PMP_ENTRY_NULLPTR_HIGH_NAPOT,
        ((riscv_pmpcfg_t){
            .read            = false,
            .write           = false,
            .exec            = false,
            .addr_match_mode = RISCV_PMPCFG_NAPOT,
            ._reserved       = 0,
            .lock            = true,
        })
    );

    // Add ROM write-protect range.
    riscv_pmpaddr_write(PMP_ENTRY_ROM_WP_NAPOT, riscv_pmpaddr_calc_napot(PMP_BASE_ROM_WP, PMP_SIZE_ROM_WP));
    riscv_pmpcfg_set(
        PMP_ENTRY_ROM_WP_NAPOT,
        ((riscv_pmpcfg_t){
            .read            = true,
            .write           = false,
            .exec            = true,
            .addr_match_mode = RISCV_PMPCFG_NAPOT,
            ._reserved       = 0,
            .lock            = true,
        })
    );

    // Add FLASH write-protect range.
    riscv_pmpaddr_write(PMP_ENTRY_FLASH_WP_NAPOT, riscv_pmpaddr_calc_napot(PMP_BASE_FLASH_WP, PMP_SIZE_FLASH_WP));
    riscv_pmpcfg_set(
        PMP_ENTRY_FLASH_WP_NAPOT,
        ((riscv_pmpcfg_t){
            .read            = true,
            .write           = false,
            .exec            = true,
            .addr_match_mode = RISCV_PMPCFG_NAPOT,
            ._reserved       = 0,
            .lock            = true,
        })
    );

    // Add user global permissions.
    // TODO: These permissions will be NONE later, but are ALL for now.
    riscv_pmpaddr_write(PMP_ENTRY_USER_GLOBAL_NAPOT, RISCV_PMPADDR_NAPOT_GLOBAL);
    riscv_pmpcfg_set(
        PMP_ENTRY_USER_GLOBAL_NAPOT,
        ((riscv_pmpcfg_t){
            .read            = true,
            .write           = true,
            .exec            = true,
            .addr_match_mode = RISCV_PMPCFG_NAPOT,
            ._reserved       = 0,
            .lock            = false,
        })
    );
}

// Set the range of external RAM currently assigned to userland.
void memprotect_set_user_extram(size_t base, size_t top) {
    bool mie = interrupt_disable();

    // Disable existing entry.
    riscv_pmpcfg_clear(PMP_ENTRY_USER_EXTRAM_TOR);
    // Write new addresses.
    riscv_pmpaddr_write(PMP_ENTRY_USER_EXTRAM_BASE, base);
    riscv_pmpaddr_write(PMP_ENTRY_USER_EXTRAM_TOR, top);
    // Set PMP region type.
    riscv_pmpcfg_set(
        PMP_ENTRY_USER_EXTRAM_TOR,
        ((riscv_pmpcfg_t){
            .read            = true,
            .write           = true,
            .exec            = true,
            .addr_match_mode = RISCV_PMPCFG_TOR,
            ._reserved       = 0,
            .lock            = false,
        })
    );

    if (mie)
        interrupt_enable();
}

// Set the range of SRAM currently assigned to userland.
void memprotect_set_user_sram(size_t base, size_t top) {
    bool mie = interrupt_disable();

    // Disable existing entry.
    riscv_pmpcfg_clear(PMP_ENTRY_USER_SRAM_TOR);
    // Write new addresses.
    riscv_pmpaddr_write(PMP_ENTRY_USER_SRAM_BASE, base);
    riscv_pmpaddr_write(PMP_ENTRY_USER_SRAM_TOR, top);
    // Set PMP region type.
    riscv_pmpcfg_set(
        PMP_ENTRY_USER_SRAM_TOR,
        ((riscv_pmpcfg_t){
            .read            = true,
            .write           = true,
            .exec            = true,
            .addr_match_mode = RISCV_PMPCFG_TOR,
            ._reserved       = 0,
            .lock            = false,
        })
    );

    if (mie)
        interrupt_enable();
}

// Set the range of flash currently assigned to userland.
void memprotect_set_user_flash(size_t base, size_t top) {
    bool mie = interrupt_disable();

    // Disable existing entry.
    riscv_pmpcfg_clear(PMP_ENTRY_USER_FLASH_TOR);
    // Write new addresses.
    riscv_pmpaddr_write(PMP_ENTRY_USER_FLASH_BASE, base);
    riscv_pmpaddr_write(PMP_ENTRY_USER_FLASH_TOR, top);
    // Set PMP region type.
    riscv_pmpcfg_set(
        PMP_ENTRY_USER_FLASH_TOR,
        ((riscv_pmpcfg_t){
            .read            = true,
            .write           = true,
            .exec            = true,
            .addr_match_mode = RISCV_PMPCFG_TOR,
            ._reserved       = 0,
            .lock            = false,
        })
    );

    if (mie)
        interrupt_enable();
}
