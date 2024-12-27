
// SPDX-License-Identifier: MIT

#pragma once

#include "memprotect.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define MMU_BITS_PER_LEVEL     9
#define MMU_PAGE_SIZE          0x1000LLU
#define MMU_SUPPORT_SUPERPAGES 1
#define MMU_LEAF_MAX_LEVEL     2

// Different types of AMD64 MMU PTE.
typedef union {
    struct {
        // Present.
        size_t p      : 1;
        // Read/write.
        size_t rw     : 1;
        // User/supervisor.
        size_t us     : 1;
        // Write-through.
        size_t pwt    : 1;
        // Disabled cache.
        size_t pcd    : 1;
        // Accessed.
        size_t a      : 1;
        // Dirty.
        size_t d      : 1;
        // Level 1+: Page size, level 0: page attribute table.
        size_t pat_ps : 1;
        // Global.
        size_t g      : 1;
        // Available for kernel use.
        size_t avl0   : 3;
        // Address bits.
        size_t addr   : 40;
        // Available for kernel use.
        size_t avl1   : 7;
        // Protection key.
        size_t pk     : 4;
        // Execute disable.
        size_t xd     : 1;
    };
    size_t val;
} mmu_pte_t;

// Page table walk result.
typedef struct {
    // Physical address of last loaded PTA.
    size_t    paddr;
    // Last loaded PTE.
    mmu_pte_t pte;
    // Page table level of PTE.
    uint8_t   level;
    // Whether the subject page was found.
    bool      found;
    // Whether the virtual address is valid.
    bool      vaddr_valid;
} mmu_walk_t;



// Virtual address offset used for HHDM.
extern size_t mmu_hhdm_vaddr;
// Virtual address offset of the higher half.
extern size_t mmu_high_vaddr;
// Size of a "half".
extern size_t mmu_half_size;
// How large to make the HHDM, rounded up to pages.
extern size_t mmu_hhdm_size;
// Number of page table levels.
extern int    mmu_levels;
// Virtual page number offset used for HHDM.
#define mmu_hhdm_vpn   (mmu_hhdm_vaddr / MMU_PAGE_SIZE)
// Virtual page number of the higher half.
#define mmu_high_vpn   (mmu_high_vaddr / MMU_PAGE_SIZE)
// Virtual page size of a "half".
#define mmu_half_pages (mmu_half_size / MMU_PAGE_SIZE)



// Whether a certain DTB MMU type is supported.
bool mmu_dtb_supported(char const *type);

// MMU-specific init code.
void mmu_early_init();
// MMU-specific init code.
void mmu_init();

// Get the index from the VPN for a given page table level.
static inline size_t mmu_vpn_part(size_t vpn, int pt_level) {
    return (vpn >> (9 * pt_level)) & 0x1ff;
}

// Read a PTE from the page table.
mmu_pte_t mmu_read_pte(size_t pte_paddr);
// Write a PTE to the page table.
void      mmu_write_pte(size_t pte_paddr, mmu_pte_t pte);

// Create a new leaf node PTE.
static inline mmu_pte_t mmu_pte_new_leaf(size_t ppn, int level, uint32_t flags) {
    mmu_pte_t pte = {0};
    pte.addr      = ppn;
    pte.xd        = !(flags & MEMPROTECT_FLAG_X);
    pte.rw        = !!(flags & MEMPROTECT_FLAG_W);
    pte.us        = !(flags & MEMPROTECT_FLAG_KERNEL);
    pte.g         = !!(flags & MEMPROTECT_FLAG_GLOBAL);
    pte.pcd       = !!(flags & (MEMPROTECT_FLAG_NC | MEMPROTECT_FLAG_IO));
    pte.pwt       = !!(flags & MEMPROTECT_FLAG_IO);
    pte.a         = 1;
    pte.d         = 1;
    pte.p         = 1;
    if (level) {
        pte.pat_ps = 1;
    }
    return pte;
}
// Create a new internal PTE.
static inline mmu_pte_t mmu_pte_new(size_t ppn, int level) {
    mmu_pte_t pte = {0};
    pte.addr      = ppn;
    pte.p         = 1;
    return pte;
}
// Creates a invalid PTE.
#define MMU_PTE_NULL ((mmu_pte_t){0})

// Whether a PTE's valid/present bit is set.
static inline bool mmu_pte_is_valid(mmu_pte_t pte, int level) {
    (void)level;
    return pte.p;
}
// Whether a PTE represents a leaf node.
static inline bool mmu_pte_is_leaf(mmu_pte_t pte, int level) {
    return level == 0 || pte.pat_ps;
}
// Get memory protection flags encoded in PTE.
static inline uint32_t mmu_pte_get_flags(mmu_pte_t pte, int level) {
    (void)level;
    return (!pte.us * MEMPROTECT_FLAG_KERNEL) | (pte.g * MEMPROTECT_FLAG_GLOBAL) | (!pte.xd * MEMPROTECT_FLAG_X) |
           (pte.rw * MEMPROTECT_FLAG_W) | MEMPROTECT_FLAG_R;
}
// Get physical page number encoded in PTE.
static inline size_t mmu_pte_get_ppn(mmu_pte_t pte, int level) {
    if (level) {
        return pte.addr & ~1;
    } else {
        return pte.addr;
    }
}

// Enable supervisor access to user memory.
static inline void mmu_enable_sum() {
    // TODO.
}
// Disable supervisor access to user memory.
static inline void mmu_disable_sum() {
    // TODO.
}



// Notify the MMU of global mapping changes.
static inline void mmu_vmem_fence() {
    // TODO.
}
