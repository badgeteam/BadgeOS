
// SPDX-License-Identifier: MIT

#pragma once

#include "attributes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Maximum number of buses.
#define PCIE_MAX_BUSES         256
// Devices per bus.
#define PCIE_DEV_PER_BUS       32
// Functions per device.
#define PCIE_FUNC_PER_DEV      8
// ECAM size per bus.
#define PCIE_ECAM_SIZE_PER_BUS (PCIE_DEV_PER_BUS * PCIE_FUNC_PER_DEV * 4096)



// PCIe command register.
typedef union {
    struct {
        // Unused.
        uint16_t          : 2;
        // Bus master / DMA enable.
        uint16_t dma_en   : 1;
        // Does not apply to PCIe.
        uint16_t          : 3;
        // Parity error response.
        uint16_t par_resp : 1;
        // Does not apply to PCIe.
        uint16_t          : 1;
        // SERR# enable.
        uint16_t serr_en  : 1;
        // Does not apply to PCIe.
        uint16_t          : 1;
        // Interrupt disable.
        uint16_t irq_dis  : 1;
    };
    uint16_t val;
} pcie_cmdr_t;

// PCIe status register.
typedef union {
    struct {
        // Unused.
        uint16_t                   : 3;
        // Has extended capabilities list.
        uint16_t extcap            : 1;
        // Does not apply to PCIe.
        uint16_t                   : 2;
        // Master detected parity error.
        uint16_t md_parity_err     : 1;
        // Does not apply to PCIe.
        uint16_t                   : 1;
        // Singalled target abort.
        uint16_t sig_target_abort  : 1;
        // Received target abort.
        uint16_t recv_target_abort : 1;
        // Received master abort.
        uint16_t recv_master_abort : 1;
        // Signalled system error.
        uint16_t sig_sys_err       : 1;
        // Detected parity error.
        uint16_t parity_err        : 1;
    };
    uint16_t val;
} pcie_statr_t;

// PCIe common header layout.
typedef struct {
    // Vendor ID.
    uint16_t              vendor_id;
    // Device ID.
    uint16_t              device_id;
    // Command register.
    VOLATILE pcie_cmdr_t  command;
    // Status register.
    VOLATILE pcie_statr_t status;
    // Revision ID.
    uint8_t               rev;
    // TODO: Figure out what class code means.
    uint8_t               classcode[3];
    // Cache line size.
    uint8_t               cache_line_size;
    // Master latency timer (does not apply to PCIe).
    uint8_t               _reserved0;
    // Header type.
    uint8_t               hdr_type;
    // TODO: Figure out what BIST means.
    uint8_t               bist;
} pcie_hdr_com_t;
_Static_assert(sizeof(pcie_hdr_com_t) == 0x10, "Size of `pcie_hdr_com_t` must be 0x10");

// PCIe type 0 (device) header layout.
typedef struct {
    // Header data common to type 0 and type 1.
    pcie_hdr_com_t    common;
    // Base address registers.
    VOLATILE uint32_t bar[6];
    // Cardbus CIS pointer.
    uint32_t          cis_ptr;
    // Subsystem vendor ID.
    uint16_t          sub_vendor_id;
    // Subsystem ID.
    uint16_t          sub_id;
    // Expansion ROM base address.
    uint32_t          xrom_base;
    // Capabilities pointer.
    uint8_t           cap_ptr;
    // Reserved.
    uint8_t           _reserved0[7];
    // Interrupt line.
    VOLATILE uint8_t  irq_line;
    // Interrupt pin.
    VOLATILE uint8_t  irq_pin;
    // Does not apply to PCIe.
    uint8_t           _reserved1[2];
} pcie_hdr_dev_t;
_Static_assert(sizeof(pcie_hdr_dev_t) == 0x40, "Size of `pcie_hdr_dev_t` must be 0x40");

// PCIe type 1 (switch) header layout.
typedef struct {
    // Header data common to type 0 and type 1.
    pcie_hdr_com_t    common;
    // Base address registers.
    VOLATILE uint32_t bar[2];
    // Primary bus number (not used by PCIe).
    uint8_t           _reserved0;
    // Secondary bus number.
    uint8_t           secondary_bus;
    // Subordinate bus number.
    uint8_t           subordinate_bus;
    // Secondary latency timer (not used by PCIe).
    uint8_t           _reserved1;
    // I/O base lower 8 bits.
    uint8_t           io_base_lo;
    // I/O limit lower 8 bits.
    uint8_t           io_limit_lo;
    // Secondary status.
    VOLATILE uint16_t secondary_status;
    // Memory base.
    uint16_t          mem_base;
    // Memory limit.
    uint16_t          mem_limit;
    // Prefetchable memory base lower 16 bits.
    uint16_t          pf_base_lo;
    // Prefetchable memory limit lower 16 bits.
    uint16_t          pf_limit_lo;
    // Prefetchable base upper 32 bits.
    uint32_t          pf_base_hi;
    // Prefetchable limit upper 32 bits.
    uint32_t          pf_limit_hi;
    // I/O base upper 16 bits.
    uint16_t          io_base_hi;
    // I/O limit upper 16 bits.
    uint16_t          io_limit_hi;
    // Capabilities pointer.
    uint8_t           cap_ptr;
    // Reserved.
    uint8_t           _reserved2[3];
    // Expansion rom base address.
    uint32_t          xrom_base;
    // Interrupt line.
    VOLATILE uint8_t  irq_line;
    // Interrupt pin.
    VOLATILE uint8_t  irq_pin;
    // Does not apply to PCIe.
    uint8_t           _reserved3[2];
} pcie_hdr_sw_t;
_Static_assert(sizeof(pcie_hdr_sw_t) == 0x40, "Size of `pcie_hdr_sw_t` must be 0x40");



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
    size_t   cpu_paddr;
    // PCIe BAR space address.
    uint32_t pci_addr[3];
    // Region length in bytes.
    size_t   length;
    /* ==== initialized by generic driver ==== */
    // CPU virtual address.
    size_t   cpu_vaddr;
} pcie_bar_range_t;

// PCIe controller information.
typedef struct {
    /* ==== initialized by controller-specific driver ==== */
    // PCIe controller type.
    pcie_ctype_t      type;
    // Number of ranges mapping CPU paddr to BAR paddr.
    size_t            ranges_len;
    // Ranges mapping CPU paddr to BAR paddr.
    // Must be allocated with `malloc()`.
    pcie_bar_range_t *ranges;
    // First bus number.
    uint8_t           bus_start;
    // Last bus number.
    uint8_t           bus_end;
    // Configuration space (usually ECAM) CPU physical address.
    size_t            config_paddr;
    /* ==== initialized by generic driver ==== */
    // Assigned PCIe controller index.
    size_t            ctl_idx;
    // Configuration space CPU virtual address.
    size_t            config_vaddr;
    // Offset into the BAR that of the corresponding CPU virtual address.
    size_t            bar_offset;
    // Size of the BAR.
    size_t            bar_size;
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



// Enumerate devices via ECAM.
void  pcie_ecam_detect();
// Get the ECAM virtual address for a device.
void *pcie_ecam_vaddr(pcie_addr_t addr);



// Get the ECAM virtual address for a device.
static inline void *pcie_ecam_vaddr3(uint8_t bus, uint8_t dev, uint8_t func) {
    return pcie_ecam_vaddr((pcie_addr_t){bus, dev, func});
}
