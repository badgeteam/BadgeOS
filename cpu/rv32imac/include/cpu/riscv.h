
// SPDX-License-Identifier: MIT

#pragma once



/* ==== RISC-V MSTATUS DEFINITION ==== */

#define RV32_MSTATUS_SIE_BIT      1
#define RV32_MSTATUS_MIE_BIT      3
#define RV32_MSTATUS_SPIE_BIT     5
#define RV32_MSTATUS_UBE_BIT      6
#define RV32_MSTATUS_MPIE_BIT     7
#define RV32_MSTATUS_SPP_BIT      8
#define RV32_MSTATUS_VS_BASE_BIT  9  // ,10
#define RV32_MSTATUS_MPP_BASE_BIT 11 // ,12
#define RV32_MSTATUS_FS_BASE_BIT  13 // ,14
#define RV32_MSTATUS_XS_BASE_BIT  15 // ,16
#define RV32_MSTATUS_MPRV_BIT     17
#define RV32_MSTATUS_SUM_BIT      18
#define RV32_MSTATUS_MXR_BIT      19
#define RV32_MSTATUS_TVM_BIT      20
#define RV32_MSTATUS_TW_BIT       21
#define RV32_MSTATUS_TSR_BIT      22
#define RV32_MSTATUS_SR_BIT       31



/* ==== RISC-V TRAP LIST ==== */

// Instruction access misaligned.
#define RV_TRAP_IALIGN   0x00
// Instruction access fault.
#define RV_TRAP_IACCESS  0x01
// Illegal instruction.
#define RV_TRAP_IILLEGAL 0x02
// Trace / breakpoint trap.
#define RV_TRAP_EBREAK   0x03
// Load access misaligned.
#define RV_TRAP_LALIGN   0x04
// Load access fault.
#define RV_TRAP_LACCESS  0x05
// Store / AMO access misaligned.
#define RV_TRAP_SALIGN   0x06
// Store / AMO access fault.
#define RV_TRAP_SACCESS  0x07
// ECALL from U-mode.
#define RV_TRAP_U_ECALL  0x08
// ECALL from S-mode.
#define RV_TRAP_S_ECALL  0x09
// ECALL from M-mode.
#define RV_TRAP_M_ECALL  0x0B
// Instruction page fault.
#define RV_TRAP_IPAGE    0x0C
// Load page fault.
#define RV_TRAP_LPAGE    0x0D
// Store / AMO page fault.
#define RV_TRAP_SPAGE    0x0F
