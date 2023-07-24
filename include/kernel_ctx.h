
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct kernel_ctx_t kernel_ctx_t;

// Get the current kernel context.
static inline kernel_ctx_t *kernel_ctx_get();

// Get the outstanding context swap target, if any.
static inline kernel_ctx_t *kernel_ctx_switch_get();

// Set the context swap target to swap to before exiting the trap/interrupt
// handler.
static inline void kernel_ctx_switch_set(kernel_ctx_t *switch_to);

// Print a register dump given kernel_ctx_t.
void kernel_ctx_dump(kernel_ctx_t const *ctx);
// Print a register dump of the current registers.
void kernel_cur_regs_dump();

#include "cpu/kernel_ctx.h"
