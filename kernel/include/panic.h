
// SPDX-License-Identifier: MIT

#pragma once

#include "attributes.h"

// Call this function when and only when the kernel has encountered a fatal error.
// Prints register dump for current kernel context and jumps to `panic_poweroff`.
void panic_abort() NORETURN;
// Call this function when and only when the kernel has encountered a fatal error.
// Immediately power off or reset the system.
void panic_poweroff() NORETURN;
