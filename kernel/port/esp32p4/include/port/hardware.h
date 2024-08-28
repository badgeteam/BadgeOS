
// SPDX-License-Identifier: MIT

#pragma once



/* ==== CPU INFO ==== */

// Kernel runs in M-mode instead of S-mode.
#define RISCV_M_MODE_KERNEL    1
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

// Number of interrupt channels (excluding trap handler) in the vector table.
#define RISCV_VT_INT_COUNT   47
// Number of padding words in the vector table.
#define RISCV_VT_PADDING     16
// Bitmask for interrupt cause.
#define RISCV_VT_ICAUSE_MASK 63
// Bitmask for trap cause.
#define RISCV_VT_TCAUSE_MASK 31
