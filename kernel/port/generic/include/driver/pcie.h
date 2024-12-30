
// SPDX-License-Identifier: MIT

#pragma once

#include "attributes.h"
#include "driver/pcie/bar.h"
#include "driver/pcie/class.h"
#include "driver/pcie/confspace.h"

// Maximum number of buses.
#define PCIE_MAX_BUSES         256
// Devices per bus.
#define PCIE_DEV_PER_BUS       32
// Functions per device.
#define PCIE_FUNC_PER_DEV      8
// ECAM size per bus.
#define PCIE_ECAM_SIZE_PER_BUS (PCIE_DEV_PER_BUS * PCIE_FUNC_PER_DEV * 4096)



// Information gleemed from PCI base address register.
typedef struct {
    size_t paddr;
    size_t len;
    bool   is_io;
    bool   is_64bit;
    bool   prefetch;
} pci_bar_info_t;



// PCIe controller types.
typedef enum {
    // Generic PCIe with ECAM configuration interface.
    PCIE_CTYPE_GENERIC_ECAM,
    // SiFive FU740 PCIe based on Synopsys DesignWare PCIe.
    PCIE_CTYPE_SIFIVE_FU740,
} pcie_ctype_t;

// Ranges mapping CPU paddr to BAR paddr.
typedef struct {
    /* ==== initialized by controller-specific driver ==== */
    // CPU physical address.
    size_t      cpu_paddr;
    // PCI BAR space address.
    pci_paddr_t pci_paddr;
    // Region length in bytes.
    size_t      length;
} pci_bar_range_t;

// PCIe controller information.
typedef struct {
    /* ==== initialized by controller-specific driver ==== */
    // PCIe controller type.
    pcie_ctype_t     type;
    // Number of ranges mapping CPU paddr to BAR paddr.
    size_t           ranges_len;
    // Ranges mapping CPU paddr to BAR paddr.
    // Must be allocated with `malloc()`.
    pci_bar_range_t *ranges;
    // First bus number.
    uint8_t          bus_start;
    // Last bus number.
    uint8_t          bus_end;
    // Configuration space (usually ECAM) CPU physical address.
    size_t           config_paddr;
    /* ==== initialized by generic driver ==== */
    // Assigned PCIe controller index.
    size_t           ctl_idx;
    // Configuration space CPU virtual address.
    size_t           config_vaddr;
} pcie_controller_t;


// PCIe function address.
// Includes controller index.
typedef struct {
    // PCIe cotroller's bus number.
    uint8_t bus;
    // PCIe bus's device number.
    uint8_t dev;
    // PCIe device's function number.
    uint8_t func;
} pcie_addr_t;

// Mapped PCI BAR.
typedef struct {
    // Relevant BAR info, including length.
    pci_bar_info_t bar;
    // CPU virtual address / pointer.
    void          *pointer;
} pci_bar_handle_t;



// Enumerate devices via ECAM.
void             pcie_ecam_detect();
// Get the ECAM virtual address for a device.
void            *pcie_ecam_vaddr(pcie_addr_t addr);
// Get info from a BAR register.
pci_bar_info_t   pci_bar_info(VOLATILE pci_bar_t *bar);
// Map a BAR into CPU virtual memory.
pci_bar_handle_t pci_bar_map(VOLATILE pci_bar_t *bar);



// Get the ECAM virtual address for a device.
static inline void *pcie_ecam_vaddr3(uint8_t bus, uint8_t dev, uint8_t func) {
    return pcie_ecam_vaddr((pcie_addr_t){bus, dev, func});
}
