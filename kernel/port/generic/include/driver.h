
// SPDX-License-Identifier: MIT

#pragma once



#include "driver/pcie.h"
#include "port/dtb.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



// Init function for devices detected from DTB.
typedef void (*driver_dtb_init_t)(dtb_handle_t *dtb, dtb_node_t *node, uint32_t addr_cells, uint32_t size_cells);
// Init function for devices detected from PCI / PCIe.
typedef void (*driver_pci_init_t)(pcie_addr_t addr);

// Supported device driver types.
typedef enum {
    DRIVER_TYPE_DTB = 1,
    DRIVER_TYPE_PCI = 2,
} driver_type_t;

// Generic driver information.
typedef struct {
    // Driver type.
    driver_type_t type;
    union {
        struct {
            // PCI device class code.
            uint8_t           pci_class[3];
            // Init from PCI / PCIe.
            driver_pci_init_t pci_init;
        };
        struct {
            // Number of DTB compatible keywords.
            size_t                   dtb_supports_len;
            // Supported DTB compatible keywords.
            char const *const *const dtb_supports;
            // Init from DTB.
            driver_dtb_init_t        dtb_init;
        };
    };
} driver_t;

// Start of driver list.
extern driver_t const start_drivers[] asm("__start_drivers");
// End of driver list.
extern driver_t const stop_drivers[] asm("__stop_drivers");

// Define a new driver.
#define DRIVER_DECL(name) __attribute((section(".drivers"))) driver_t const name
