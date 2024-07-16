
// SPDX-License-Identifier: MIT

#pragma once

#include "port/dtb.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Initialise the SMP subsystem.
void smp_init(dtb_handle_t *dtb);
