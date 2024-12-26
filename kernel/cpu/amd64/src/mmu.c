
// SPDX-License-Identifier: MIT

#include "cpu/mmu.h"

#include "assertions.h"

_Static_assert(MEMMAP_PAGE_SIZE == MMU_PAGE_SIZE, "MEMMAP_PAGE_SIZE must equal MMU_PAGE_SIZE");



// Virtual address offset used for HHDM.
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



// Whether a certain DTB MMU type is supported.
bool mmu_dtb_supported(char const *type) {
    // TODO.
    return true;
}


// Read a PTE from the page table.
mmu_pte_t mmu_read_pte(size_t pte_paddr) {
    return (mmu_pte_t){.val = pmem_load(pte_paddr)};
}

// Write a PTE to the page table.
void mmu_write_pte(size_t pte_paddr, mmu_pte_t pte) {
    pmem_store(pte_paddr, pte.val);
}
