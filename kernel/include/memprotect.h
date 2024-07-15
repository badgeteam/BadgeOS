
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MEMPROTECT_FLAG_R      0x00000001
#define MEMPROTECT_FLAG_W      0x00000002
#define MEMPROTECT_FLAG_RW     0x00000003
#define MEMPROTECT_FLAG_X      0x00000004
#define MEMPROTECT_FLAG_RX     0x00000005
#define MEMPROTECT_FLAG_WX     0x00000006
#define MEMPROTECT_FLAG_RWX    0x00000007
// Global mapping; visible in all address spaces.
#define MEMPROTECT_FLAG_GLOBAL 0x00000010
// Kernel mapping; accessible to kernel mode instead of user mode.
#define MEMPROTECT_FLAG_KERNEL 0x00000020
// I/O mapping; if possible, mark region as I/O.
#define MEMPROTECT_FLAG_IO     0x00000040
// Non-cachable; if possible, mark region as non-cacheable.
#define MEMPROTECT_FLAG_NC     0x00000080

#include "port/hardware_allocation.h"
#include "port/memprotect.h"
#include "process/types.h"


#if MEMMAP_VMEM
// For systems with VMEM: global MMU context.
extern mpu_ctx_t mpu_global_ctx;
// Virtual to physical lookup results.
typedef struct {
    // Memory protection flags.
    uint32_t flags;
    // Physical address.
    size_t   paddr;
    // Page base vaddr.
    size_t   page_vaddr;
    // Page base paddr.
    size_t   page_paddr;
    // Page size.
    size_t   page_size;
} virt2phys_t;
// Lookup virtual address to physical address.
virt2phys_t memprotect_virt2phys(mpu_ctx_t *ctx, size_t vaddr);
// Allocare a kernel virtual address to a certain physical address.
// Does not map anything to this location.
size_t      memprotect_alloc_vaddr(size_t len);
// Free a virtual address range allocated with `memprotect_alloc_vaddr`.
void        memprotect_free_vaddr(size_t vaddr);
#endif

// Initialise memory protection driver.
void memprotect_early_init();
// Initialise memory protection driver.
void memprotect_init();
// Create a memory protection context.
void memprotect_create(mpu_ctx_t *ctx);
// Clean up a memory protection context.
void memprotect_destroy(mpu_ctx_t *ctx);
// Add a memory protection region for user memory.
bool memprotect_u(proc_memmap_t *new_mm, mpu_ctx_t *ctx, size_t vaddr, size_t paddr, size_t length, uint32_t flags);
// Add a memory protection region for kernel memory.
bool memprotect_k(size_t vaddr, size_t paddr, size_t length, uint32_t flags);
// Commit pending memory protections, if any.
void memprotect_commit(mpu_ctx_t *ctx);
// Swap in memory protections for the current thread.
void memprotect_swap_from_isr();
// Swap in memory protections for a given context.
void memprotect_swap(mpu_ctx_t *ctx);
