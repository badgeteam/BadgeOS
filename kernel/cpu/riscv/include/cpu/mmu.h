
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/riscv.h"
#include "memprotect.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define MMU_BITS_PER_LEVEL     9
#define MMU_PAGE_SIZE          0x1000LLU
#define MMU_SUPPORT_SUPERPAGES 1

// PBMT modes.
typedef enum {
    // Use values from applicable PMA(s).
    RISCV_PBMT_PMA,
    // Non-cacheable idempotent main memory.
    RISCV_PBMT_NC,
    // Non-cacheable strongly-ordered I/O memory.
    RISCV_PBMT_IO,
} riscv_pbmt_t;

// Address translation modes.
typedef enum {
    // Direct-mapped; virtual memory disabled.
    RISCV_SATP_BARE = 0,
    // RISC-V page-based 39-bit virtual memory.
    RISCV_SATP_SV39 = 8,
    // RISC-V page-based 48-bit virtual memory.
    RISCV_SATP_SV48,
    // RISC-V page-based 57-bit virtual memory.
    RISCV_SATP_SV57,
} riscv_satp_mode_t;

// RISC-V MMU page table entry.
typedef union {
    struct {
        // Valid entry; entry is ignored when 0.
        size_t v    : 1;
        // Allow reading.
        size_t r    : 1;
        // Allow writing; requires reading.
        size_t w    : 1;
        // Allow executing.
        size_t x    : 1;
        // Page belongs to U-mode.
        size_t u    : 1;
        // Global page present in all page tables.
        size_t g    : 1;
        // Page has been accessed before.
        size_t a    : 1;
        // Page has been written to.
        size_t d    : 1;
        // Reserved for supervisor software.
        size_t rsw  : 2;
        // Physical page number.
        size_t ppn  : 44;
        // Reserved.
        size_t      : 7;
        // PBMT mode.
        size_t pbmt : 2;
        // NAPOT page table entry.
        size_t n    : 1;
    };
    struct {
        size_t     : 1;
        // Combined RWX flags.
        size_t rwx : 3;
    };
    size_t val;
} mmu_pte_t;

// RISC-V SATP CSR format.
typedef union {
    struct {
        // Page table root physical page number.
        size_t ppn  : 44;
        // Address-space ID.
        size_t asid : 16;
        // Address translation mode.
        size_t mode : 4;
    };
    size_t val;
} riscv_satp_t;

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
// Whether RISC-V Svpbmt is supported.
extern bool   mmu_svpbmt;
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
    (void)level;
    mmu_pte_t pte = {0};
    pte.v         = !!(flags & MEMPROTECT_FLAG_RWX);
    pte.rwx       = flags & MEMPROTECT_FLAG_RWX;
    pte.u         = !(flags & MEMPROTECT_FLAG_KERNEL);
    pte.g         = !!(flags & MEMPROTECT_FLAG_GLOBAL);
    pte.a         = 1;
    pte.d         = 1;
    if (mmu_svpbmt && flags & MEMPROTECT_FLAG_IO) {
        pte.pbmt = RISCV_PBMT_IO;
    } else if (mmu_svpbmt && flags & MEMPROTECT_FLAG_NC) {
        pte.pbmt = RISCV_PBMT_NC;
    }
    pte.ppn = ppn;
    return pte;
}
// Create a new internal PTE.
static inline mmu_pte_t mmu_pte_new(size_t ppn, int level) {
    (void)level;
    mmu_pte_t pte = {0};
    pte.v         = 1;
    pte.ppn       = ppn;
    return pte;
}
// Creates a invalid PTE.
#define MMU_PTE_NULL ((mmu_pte_t){0})

// Whether a PTE's valid/present bit is set.
static inline bool mmu_pte_is_valid(mmu_pte_t pte, int level) {
    (void)level;
    return pte.v;
}
// Whether a PTE represents a leaf node.
static inline bool mmu_pte_is_leaf(mmu_pte_t pte, int level) {
    (void)level;
    return pte.rwx != 0;
}
// Get memory protection flags encoded in PTE.
static inline uint32_t mmu_pte_get_flags(mmu_pte_t pte, int level) {
    (void)level;
    return pte.rwx | (pte.g * MEMPROTECT_FLAG_GLOBAL) | (!pte.u * MEMPROTECT_FLAG_KERNEL);
}
// Get physical page number encoded in PTE.
static inline size_t mmu_pte_get_ppn(mmu_pte_t pte, int level) {
    (void)level;
    return pte.ppn;
}

// Enable supervisor access to user memory.
static inline void mmu_enable_sum() {
    asm("csrs sstatus, %0" ::"r"((1 << RISCV_STATUS_SUM_BIT) | (1 << RISCV_STATUS_MXR_BIT)));
}
// Disable supervisor access to user memory.
static inline void mmu_disable_sum() {
    asm("csrc sstatus, %0" ::"r"((1 << RISCV_STATUS_SUM_BIT) | (1 << RISCV_STATUS_MXR_BIT)));
}



// Notify the MMU of global mapping changes.
static inline void mmu_vmem_fence() {
    asm volatile("fence rw,rw" ::: "memory");
    asm volatile("sfence.vma" ::: "memory");
}
