
// SPDX-License-Identifier: MIT

#include "port/port.h"

#include "assertions.h"
#include "cpu/mmu.h"
#include "cpu/panic.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "memprotect.h"
#include "port/dtparse.h"
#include "port/hardware_allocation.h"
#include "rawprint.h"

#include <stdbool.h>

#include <limine.h>

void init_pool(void *mem_start, void *mem_end, uint32_t flags);



#define REQ __attribute__((section(".requests")))

__attribute__((section(".requests_start"))) LIMINE_REQUESTS_START_MARKER;

LIMINE_BASE_REVISION(2);

static REQ struct limine_memmap_request mm_req = {
    .id       = LIMINE_MEMMAP_REQUEST,
    .revision = 2,
};

static REQ struct limine_dtb_request dtb_req = {
    .id       = LIMINE_DTB_REQUEST,
    .revision = 2,
};

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

// CPU0 local data.
cpulocal_t port_cpu0_local;

// Early hardware initialization.
void port_early_init() {
    rawprint("\033[0m\033[2J");
    isr_ctx_get()->cpulocal = &port_cpu0_local;

    // Verify needed requests have been answered.
    if (!mm_req.response) {
        logk_from_isr(LOG_FATAL, "Limine memmap response missing");
        panic_poweroff();
    }
    if (!dtb_req.response) {
        logk_from_isr(LOG_FATAL, "Limine DTB response missing");
        logk_from_isr(LOG_FATAL, "HINT: If running QEMU, try `-M virt,acpi=off`");
        panic_poweroff();
    }
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
    logkf_from_isr(LOG_DEBUG, "Memory map:");
    size_t kernel_len         = 0;
    size_t biggest_pool_size  = 0;
    size_t biggest_pool_index = 0;
    mmu_hhdm_size             = 0;
    size_t usable_len         = 0;
    size_t reclaim_len        = 0;
    for (uint64_t i = 0; i < mem->entry_count; i++) {
        struct limine_memmap_entry *entry = mem->entries[i];
        logkf_from_isr(
            LOG_DEBUG,
            "%{u64;x}-%{u64;x} %{cs}",
            entry->base,
            entry->base + entry->length - 1,
            types[entry->type]
        );
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            usable_len += entry->length;
        } else if (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
                   entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE) {
            reclaim_len += entry->length;
        }
        if (entry->type == LIMINE_MEMMAP_KERNEL_AND_MODULES) {
            kernel_len = entry->length;
        }
        // Workaround: Limine has a bug where the DTB is in usable, not reclaimable, memory.
        if ((size_t)dtb_req.response->dtb_ptr - hhdm_req.response->offset < entry->base ||
            (size_t)dtb_req.response->dtb_ptr - hhdm_req.response->offset >= entry->base + entry->length) {
            // This second check was always there and just picks the largest candidate.
            if (entry->type == LIMINE_MEMMAP_USABLE && entry->length > biggest_pool_size) {
                biggest_pool_index = i;
                biggest_pool_size  = entry->length;
            }
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

    // Report memory stats.
    logkf_from_isr(
        LOG_INFO,
        "%{size;d} MiB total memory, %{size;d} MiB of which reclaimable",
        (usable_len + reclaim_len) >> 20,
        reclaim_len >> 20
    );

    // Add biggest already available region to allocator.
    early_alloc_index                      = biggest_pool_index;
    struct limine_memmap_entry *early_pool = mem->entries[biggest_pool_index];
    logkf_from_isr(
        LOG_INFO,
        "Early pool at 0x%{size;x}-0x%{size;x}",
        early_pool->base,
        early_pool->base + early_pool->length - 1
    );
    init_pool(
        (void *)(early_pool->base + mmu_hhdm_vaddr),
        (void *)(early_pool->base + early_pool->length + mmu_hhdm_vaddr),
        0
    );
}

// Post-heap hardware initialization.
void port_postheap_init() {
    // Parse and process DTB.
    dtdump(dtb_req.response->dtb_ptr);
    dtparse(dtb_req.response->dtb_ptr);

    // Reclaim all reclaimable memory.
    size_t base = 0;
    size_t len  = 0;
    for (uint64_t i = 0; i < mm_req.response->entry_count; i++) {
        struct limine_memmap_entry *entry = mm_req.response->entries[i];
        if (i == early_alloc_index ||
            (entry->type != LIMINE_MEMMAP_USABLE && entry->type != LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE &&
             entry->type != LIMINE_MEMMAP_ACPI_RECLAIMABLE)) {
            continue;
        }
        if (entry->base != base + len) {
            if (len > 16 * MEMMAP_PAGE_SIZE) {
                logkf_from_isr(LOG_DEBUG, "Adding memory at 0x%{size;x}-0x%{size;x}", base, base + len - 1);
                init_pool((void *)(base + mmu_hhdm_vaddr), (void *)(base + len + mmu_hhdm_vaddr), 0);
            }
            base = entry->base;
            len  = entry->length;
        } else {
            len += entry->length;
        }
    }
    if (len >= 16 * MEMMAP_PAGE_SIZE) {
        logkf_from_isr(LOG_DEBUG, "Adding memory at 0x%{size;x}-0x%{size;x}", base, base + len - 1);
        init_pool((void *)(base + mmu_hhdm_vaddr), (void *)(base + len + mmu_hhdm_vaddr), 0);
    }
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

// Power off.
void port_poweroff(bool restart) {
    (void)restart;
    irq_disable();
    logkf_from_isr(LOG_INFO, "TODO: port_poweroff() is a stub");
    while (1) asm("");
}
