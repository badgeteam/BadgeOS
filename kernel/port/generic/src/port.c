
// SPDX-License-Identifier: MIT

#include "port/port.h"

#include "assertions.h"
#include "cpu/mmu.h"
#include "cpu/panic.h"
#include "limine.h"
#include "memprotect.h"
#include "port/hardware_allocation.h"
#include "rawprint.h"

#include <stdbool.h>

void init_pool(void *mem_start, void *mem_end, uint32_t flags);



#define REQ __attribute__((section(".requests")))

__attribute__((section(".requests_start"))) LIMINE_REQUESTS_START_MARKER;

LIMINE_BASE_REVISION(2);

static REQ struct limine_memmap_request mm_req = {
    .id       = LIMINE_MEMMAP_REQUEST,
    .revision = 2,
};

// static REQ struct limine_dtb_request dtb_req = {
//     .id       = LIMINE_DTB_REQUEST,
//     .revision = 2,
// };

static REQ struct limine_kernel_address_request addr_req = {
    .id       = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 2,
};

static REQ struct limine_hhdm_request hhdm_req = {
    .id       = LIMINE_HHDM_REQUEST,
    .revision = 2,
};

__attribute__((section(".requests_end"))) LIMINE_REQUESTS_END_MARKER;



// Memory map entry selected to be early alloc pool.
static size_t early_alloc_index;

// Early hardware initialization.
void port_early_init() {
    rawprint("\033[0m\033[2J");

    // Verify needed requests have been answered.
    if (!mm_req.response) {
        logk_from_isr(LOG_FATAL, "Limine memmap response missing");
        panic_poweroff();
    }
    // if (!dtb_req.response) {
    //     logk_from_isr(LOG_FATAL, "Limine DTB response missing");
    //     panic_poweroff();
    // }
    if (!addr_req.response) {
        logk_from_isr(LOG_FATAL, "Limine kernel address response missing");
        panic_poweroff();
    }
    if (!hhdm_req.response) {
        logk_from_isr(LOG_FATAL, "Limine HHDM response missing");
        panic_poweroff();
    }

    // Print memory map.
    struct limine_memmap_response *mem     = mm_req.response;
    char const *const              types[] = {
        [LIMINE_MEMMAP_USABLE]                 = "Usable",
        [LIMINE_MEMMAP_RESERVED]               = "Reserved",
        [LIMINE_MEMMAP_ACPI_RECLAIMABLE]       = "ACPI Reclaimable",
        [LIMINE_MEMMAP_ACPI_NVS]               = "ACPI NVS",
        [LIMINE_MEMMAP_BAD_MEMORY]             = "Bad",
        [LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE] = "Bootloader reclaimable",
        [LIMINE_MEMMAP_KERNEL_AND_MODULES]     = "Kernel",
        [LIMINE_MEMMAP_FRAMEBUFFER]            = "Framebuffer",
    };
    logkf_from_isr(LOG_INFO, "Memory map:");
    size_t kernel_len         = 0;
    size_t biggest_pool_size  = 0;
    size_t biggest_pool_index = 0;
    mmu_hhdm_size             = 0;
    for (uint64_t i = 0; i < mem->entry_count; i++) {
        struct limine_memmap_entry *entry = mem->entries[i];
        logkf_from_isr(
            LOG_INFO,
            "%{u64;x}-%{u64;x} %{cs}",
            entry->base,
            entry->base + entry->length - 1,
            types[entry->type]
        );
        if (entry->type == LIMINE_MEMMAP_KERNEL_AND_MODULES) {
            if (kernel_len) {
                logk_from_isr(LOG_FATAL, "Duplicate kernel in memmap");
                panic_poweroff();
            }
            kernel_len = entry->length;
        }
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length > biggest_pool_size) {
            biggest_pool_index = i;
            biggest_pool_size  = entry->length;
        }
        if (entry->type != LIMINE_MEMMAP_BAD_MEMORY && entry->type != LIMINE_MEMMAP_RESERVED) {
            mmu_hhdm_size = entry->base + entry->length;
        }
    }

    // Pass info to memory protection.
    mmu_hhdm_vaddr        = hhdm_req.response->offset;
    memprotect_hhdm_pages = (mmu_hhdm_size - 1) / MMU_PAGE_SIZE + 1;
    memprotect_kernel_ppn = addr_req.response->physical_base / MMU_PAGE_SIZE;
    memprotect_kernel_vpn = addr_req.response->virtual_base / MMU_PAGE_SIZE;
    if (addr_req.response->physical_base % MMU_PAGE_SIZE) {
        logkf_from_isr(LOG_FATAL, "Kernel is not aligned to page size");
        panic_poweroff();
    }
    memprotect_kernel_pages = (kernel_len - 1) / MMU_PAGE_SIZE + 1;

    // Add biggest already available region to allocator.
    early_alloc_index                      = biggest_pool_index;
    struct limine_memmap_entry *early_pool = mem->entries[biggest_pool_index];
    init_pool(
        (void *)(early_pool->base + mmu_hhdm_vaddr),
        (void *)(early_pool->base + early_pool->length + mmu_hhdm_vaddr),
        0
    );
}

// Tell port to add other available memory to the pools.
void port_post_memprotect_init() {
    // TODO.
}

// Full hardware initialization.
void port_init() {
}

// Send a single character to the log output.
void port_putc(char msg) {
    register char a0 asm("a0") = msg;
    // SBI console putchar.
    asm("li a7, 1; ecall" ::"r"(a0));
}
