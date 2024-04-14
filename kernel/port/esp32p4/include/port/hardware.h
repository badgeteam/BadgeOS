
// SPDX-License-Identifier: MIT

#pragma once



/* ==== CPU INFO ==== */

// Number of PMP regions supported by the CPU.
#define RISCV_PMP_REGION_COUNT 16
// Enable use of CLIC interrupt controller.
#define RISCV_USE_CLIC
// Number of CLIC interrupt priority levels.
#define RISCV_CLIC_MAX_PRIO 7
// MTVT CSR index.
#define CSR_MTVT            0x307
// MTVT CSR name.
#define CSR_MTVT_STR        "0x307"
// MINTSTATUS CSR index.
// As per CLIC specs, mintstatus CSR should be at 0xFB1, however esp32p4 implements it at 0x346
#define CSR_MINTSTATUS      0x346
// MINTSTATUS CSR name.
#define CSR_MINTSTATUS_STR  "0x346"


/* ==== SOC INFO ==== */

// Number of timer groups.
#define ESP_TIMG_COUNT       2
// Number of timers per timer group.
#define ESP_TIMG_TIMER_COUNT 1
