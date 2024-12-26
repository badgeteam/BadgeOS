
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Early hardware initialization.
void port_early_init();
// Post-heap hardware initialization.
void port_postheap_init();
// Full hardware initialization.
void port_init();
// Power off.
void port_poweroff(bool restart);
// Send a single character to the log output.
void port_putc(char msg);

// Fence data and instruction memory for executable mapping.
static inline void port_fencei() {
#ifdef __riscv
    asm("fence rw,rw");
    asm("fence.i");
#elif defined(__x86_64__)
    // TODO: Figure out which fence to use.
#endif
}
