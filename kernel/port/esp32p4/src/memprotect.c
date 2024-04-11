
// SPDX-License-Identifier: MIT

#include "memprotect.h"

#include "assertions.h"
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



// Add a memory protection region.
bool memprotect(mpu_ctx_t *ctx, size_t vaddr, size_t paddr, size_t length, uint32_t flags) {
    (void)ctx;
    (void)vaddr;
    (void)paddr;
    (void)length;
    (void)flags;
    return true;
    // Uncomment to enable PMP memory protection.
    // return vaddr == paddr && riscv_pmp_memprotect(ctx, vaddr, length, flags);
}



// Commit pending memory protections, if any.
void memprotect_commit(mpu_ctx_t *ctx) {
    (void)ctx;
}
