
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/regs.h"

// Call this function when and only when the kernel has encountered a fatal error.
// Prints register dump for current kernel context and jumps to `panic_poweroff`.
void panic_abort() __attribute__((noreturn));
// Call this function when and only when the kernel has encountered a fatal error.
// Immediately power off or reset the system.
void panic_poweroff() __attribute__((noreturn));
