
// SPDX-License-Identifier: MIT

#pragma once

#include <stddef.h>
#include <stdint.h>



/* ==== CPU INFO ==== */

// Number of PMP regions supported by the CPU.
#define RISCV_PMP_REGION_COUNT 16


/* ==== SOC INFO ==== */

// Number of timer groups.
#define ESP_TIMG_COUNT       2
// Number of timers per timer group.
#define ESP_TIMG_TIMER_COUNT 2
