
// SPDX-License-Identifier: MIT

#pragma once

#include "scheduler/scheduler.h"

#include <stddef.h>



// CPU-local data.
typedef struct {
    // Current CPU ID.
    size_t            cpuid;
    // ISR stack top.
    size_t            isr_stack_top;
    // ISR stack bottom.
    size_t            isr_stack_bottom;
    // CPU-local scheduler data.
    sched_cpulocal_t *sched;
} cpulocal_t;

// Per-CPU CPU-local data.
extern cpulocal_t *cpulocal;
