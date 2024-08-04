
// SPDX-License-Identifier: MIT

#include "driver/pcie.h"

#include "arrays.h"
#include "assertions.h"
#include "driver.h"
#include "log.h"
#include "memprotect.h"



// Detected PCIe controllers.
static pcie_controller_t ctl;



// Apply VMM mappings to make the PCIe controller accessible.
bool pcie_memprotect(pcie_controller_t *ctl) {
    size_t   bus_count   = ctl->bus_end - ctl->bus_start + 1;
    size_t   config_size = bus_count * PCIE_ECAM_SIZE_PER_BUS;
    uint32_t flags       = MEMPROTECT_FLAG_RW | MEMPROTECT_FLAG_IO;
    return memprotect_k(ctl->config_vaddr, ctl->config_paddr, config_size, flags);
}

// Initialise the PCIe controller.
// Init successful or not, takes ownership of the memory in `ctl`.
static bool pcie_controller_init() {
    // Allocate ranges of virtual memory to map the controller into.
    size_t bus_count   = ctl.bus_end - ctl.bus_start + 1;
    size_t config_size = bus_count * PCIE_ECAM_SIZE_PER_BUS;
    ctl.config_vaddr   = memprotect_alloc_vaddr(config_size);
    if (!ctl.config_vaddr) {
        free(ctl.ranges);
        return -1;
    }

    // Apply VMM mappings to make the device accessible.
    if (!pcie_memprotect(&ctl)) {
        memprotect_free_vaddr(ctl.config_vaddr);
        free(ctl.ranges);
        return -1;
    }

    // Detect devices.
    pcie_ecam_detect();
    return true;
}

// Enumerate function via ECAM.
void pcie_ecam_func_detect(uint8_t bus, uint8_t dev, uint8_t func) {
    pcie_hdr_com_t *hdr = (void *)(ctl.config_vaddr + (bus * 256 + dev * 8 + func) * 4096);
    logkf(LOG_DEBUG, "PCIe device %{u8;x}:%{u8;x}.%{u8;d} detected, type %{u8;x}", bus, dev, func, hdr->hdr_type);
}

// Enumerate device via ECAM.
void pcie_ecam_dev_detect(uint8_t bus, uint8_t dev) {
    pcie_hdr_com_t *hdr = (void *)(ctl.config_vaddr + (bus * 256 + dev * 8) * 4096);
    if (hdr->vendor_id == 0xffff) {
        return;
    }
    pcie_ecam_func_detect(bus, dev, 0);
    bool multifunc = hdr->hdr_type & 0x80;
    if (multifunc) {
        for (uint8_t func = 1; func < 8; func++) {
            hdr = (void *)(ctl.config_vaddr + (bus * 256 + dev * 8 + func) * 4096);
            if (hdr->vendor_id == 0xffff) {
                continue;
            }
            pcie_ecam_func_detect(bus, dev, func);
        }
    }
}

// Enumerate devices via ECAM.
void pcie_ecam_detect() {
    for (unsigned bus = ctl.bus_start; bus <= ctl.bus_end; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            pcie_ecam_dev_detect(bus, dev);
        }
    }
}

// Get the ECAM virtual address for a device.
void *pcie_ecam_vaddr(pcie_addr_t addr) {
    if (addr.bus < ctl.bus_start || addr.bus > ctl.bus_end) {
        return NULL;
    }
    return (void *)(ctl.config_vaddr + ((addr.bus - ctl.bus_start) * 256 + addr.dev * 8 + addr.func) * 4096);
}



// Extract ranges from DTB.
static bool pcie_dtb_ranges(dtb_handle_t *handle, dtb_node_t *node, uint32_t addr_cells, uint32_t size_cells) {
    dtb_prop_t *ranges = dtb_get_prop(handle, node, "ranges");
    if (!ranges) {
        return false;
    }

    // If ranges is empty, it is identity-mapped.
    if (ranges->content_len == 0) {
        ctl.ranges_len = 1;
        ctl.ranges     = malloc(sizeof(pcie_bar_range_t));
        if (!ctl.ranges) {
            return false;
        }
        ctl.ranges[0] = (pcie_bar_range_t){
            .cpu_paddr = 0,
            .pci_addr  = 0,
            .length    = SIZE_MAX,
            .cpu_vaddr = 0,
        };
        return true;
    }

    // TODO: Is a range always 7 cells?
    ctl.ranges_len = ranges->content_len / (4 * 7);
    ctl.ranges     = malloc(ctl.ranges_len * sizeof(pcie_bar_range_t));
    if (!ctl.ranges) {
        return false;
    }
    for (size_t i = 0; i < ctl.ranges_len; i++) {
        ctl.ranges[i] = (pcie_bar_range_t){
            .cpu_paddr = dtb_prop_read_cells(handle, ranges, i * 7 + 3, 2),
            .pci_addr =
                {
                    dtb_prop_read_cells(handle, ranges, i * 7 + 0, 2),
                    dtb_prop_read_cells(handle, ranges, i * 7 + 1, 2),
                    dtb_prop_read_cells(handle, ranges, i * 7 + 2, 2),
                },
            .length = dtb_prop_read_cells(handle, ranges, i * 7 + 6, 1),
        };
    }
    return true;
}

// DTB init for normal PCIe.
static void pcie_driver_dtbinit(dtb_handle_t *handle, dtb_node_t *node, uint32_t addr_cells, uint32_t size_cells) {
    ctl.type = PCIE_CTYPE_GENERIC_ECAM;
    if (!pcie_dtb_ranges(handle, node, addr_cells, size_cells)) {
        return;
    }
    ctl.config_paddr = dtb_read_cells(handle, node, "reg", 0, addr_cells);
    // size_t config_len = dtb_read_cells(handle, node, "reg", addr_cells, size_cells);
    // size_t bus_count  = config_len / 1048576;
    ctl.bus_start    = dtb_read_cell(handle, node, "bus-range", 0);
    ctl.bus_end      = dtb_read_cell(handle, node, "bus-range", 1);
    pcie_controller_init();
}

// DTB init for FU740 PCIe.
static void
    pcie_fu740_driver_dtbinit(dtb_handle_t *handle, dtb_node_t *node, uint32_t addr_cells, uint32_t size_cells) {
    ctl.type = PCIE_CTYPE_SIFIVE_FU740;
    pcie_dtb_ranges(handle, node, addr_cells, size_cells);
}



// Driver for normal no-nonsense PCIe.
DRIVER_DECL(pcie_driver) = {
    .dtb_supports_len = 1,
    .dtb_supports     = (char const *[]){"pci-host-ecam-generic"},
    .dtbinit          = pcie_driver_dtbinit,
};

// Driver for the factually stupid incoherent SiFive FU740 proprietary nonsense PCIe.
DRIVER_DECL(pcie_fu740_driver) = {
    .dtb_supports_len = 1,
    .dtb_supports     = (char const *[]){"sifive,fu740-pcie"},
    .dtbinit          = pcie_fu740_driver_dtbinit,
};
