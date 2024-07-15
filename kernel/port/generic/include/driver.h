
// SPDX-License-Identifier: MIT

#pragma once



#include "port/dtb.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



// Init function for devices detected from DTB.
typedef void (*driver_dtbinit_t)(dtb_handle_t *dtb, dtb_entity_t node, uint32_t addr_cells, uint32_t size_cells);

// Generic driver information.
typedef struct {
    // Number of DTB compatible keywords.
    size_t                   dtb_supports_len;
    // Supported DTB compatible keywords.
    char const *const *const dtb_supports;
    // Init from DTB.
    driver_dtbinit_t         dtbinit;
} driver_t;

// Driver list.
extern driver_t const *const drivers[];
// Number of drivers in the drivers list.
extern size_t const          drivers_len;
