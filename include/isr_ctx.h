
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct isr_ctx_t isr_ctx_t;

// Get the current kernel context.
static inline isr_ctx_t *isr_ctx_get();

// Get the outstanding context swap target, if any.
static inline isr_ctx_t *isr_ctx_switch_get();

// Set the context swap target to swap to before exiting the trap/interrupt
// handler.
static inline void isr_ctx_switch_set(isr_ctx_t *switch_to);

// Print a register dump given isr_ctx_t.
void isr_ctx_dump(isr_ctx_t const *ctx);
// Print a register dump of the current registers.
void kernel_cur_regs_dump();

#include "cpu/isr_ctx.h"
