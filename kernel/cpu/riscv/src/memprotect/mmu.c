
// SPDX-License-Identifier: MIT

#include "cpu/mmu.h"

#include "assertions.h"
#include "cpu/panic.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "limine.h"
#include "log.h"
#include "memprotect.h"
#include "page_alloc.h"
#include "port/hardware_allocation.h"

_Static_assert(MEMMAP_PAGE_SIZE == MMU_PAGE_SIZE);

// Virtual address offset currently used for HHDM.
size_t mmu_hhdm_vaddr;
// Virtual address offset of the higher half.
size_t mmu_high_vaddr;
// Size of a "half".
size_t mmu_half_size;
// How large to make the HHDM, rounded up to pages.
size_t mmu_hhdm_size;
// Number of page table levels.
int    mmu_levels;



// Load a word from physical memory.
static inline size_t pmem_load(size_t paddr) {
    assert_dev_drop(paddr < mmu_hhdm_size);
    return *(size_t volatile *)(paddr + mmu_hhdm_vaddr);
}

// Store a word to physical memory.
static inline void pmem_store(size_t paddr, size_t data) {
    assert_dev_drop(paddr < mmu_hhdm_size);
    *(size_t volatile *)(paddr + mmu_hhdm_vaddr) = data;
}



// MMU-specific init code.
void mmu_init() {
    // Read paging mode from SATP.
    riscv_satp_t satp;
    asm("csrr %0, satp" : "=r"(satp));
    mmu_levels     = satp.mode - RISCV_SATP_SV39 + 3;
    mmu_half_size  = 1LLU << (11 + 9 * mmu_levels);
    mmu_high_vaddr = -mmu_half_size;
    logkf_from_isr(LOG_DEBUG, "Paging levels:    %{d}", mmu_levels);
    logkf_from_isr(LOG_DEBUG, "Higher half addr: %{size;x}", mmu_high_vaddr);
}

// Read a PTE from the page table.
mmu_pte_t mmu_read_pte(size_t pte_paddr) {
    return (mmu_pte_t){.val = pmem_load(pte_paddr)};
}

// Write a PTE to the page table.
void mmu_write_pte(size_t pte_paddr, mmu_pte_t pte) {
    pmem_store(pte_paddr, pte.val);
}



// Swap in memory protections for the given context.
void memprotect_swap_from_isr() {
    memprotect_swap(isr_ctx_get()->mpu_ctx);
}

// Swap in memory protections for a given context.
void memprotect_swap(mpu_ctx_t *mpu) {
    mpu               = mpu ?: &mpu_global_ctx;
    bool         ie   = irq_enable(false);
    riscv_satp_t satp = {
        .ppn  = mpu->root_ppn,
        .asid = 0,
        .mode = RISCV_SATP_SV39 + mmu_levels - 3,
    };
    asm("csrw satp, %0; sfence.vma" ::"r"(satp));
    isr_ctx_get()->mpu_ctx = mpu;
    irq_enable(ie);
}
