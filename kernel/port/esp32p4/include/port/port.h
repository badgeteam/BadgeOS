
// SPDX-License-Identifier: MIT

#pragma once

#include <stddef.h>
#include <stdint.h>

// Early hardware initialization.
void port_early_init();
// Full hardware initialization.
void port_init();

// Send a single character to the log output.
void port_putc(char msg);

// Fence data and instruction memory for executable mapping.
void port_fencei();
