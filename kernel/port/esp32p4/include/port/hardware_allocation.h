
// SPDX-License-Identifier: MIT

#pragma once



/* ==== Interrupts ==== */

// Default priority for timer alarm interrupts.
#define INT_PRIO_TIMER_ALARM    11
// Default priority for timer watchdog interrupts.
#define INT_PRIO_WATCHDOG_ALARM 15
// Interrupt channel used for UART interrupts.
#define INT_PRIO_UART           14
// Interrupt channel used for I²C interrupts.
#define INT_PRIO_I2C            13
// Interrupt channel used for SPI interrupts.
#define INT_PRIO_SPI            12



/* ==== Timers ==== */

// Number of usable hardware timers.
#define TIMER_COUNT        2
// Timer used for system timekeeping.
#define TIMER_SYSTICK_NUM  0
// System tick rate in Hz.
#define TIMER_SYSTICK_RATE 1000000



/* ==== Memory protection regions ==== */

// Region size used for nullpointer protection regions.
#define PMP_SIZE_NULLPTR  0x10000000
// Region size used for ROM write protection.
#define PMP_SIZE_ROM_WP   0x00020000
// Region size used for FLASH write protection.
#define PMP_SIZE_FLASH_WP 0x04000000

// Region base used for ROM write protection.
#define PMP_BASE_ROM_WP   0x4FC00000
// Region base used for ROM write protection.
#define PMP_BASE_FLASH_WP 0x40000000

// NAPOT PMP entry for lower nullpointer protection.
#define PMP_ENTRY_NULLPTR_LOW_NAPOT  12
// NAPOT PMP entry for upper nullpointer protection.
#define PMP_ENTRY_NULLPTR_HIGH_NAPOT 13
// NAPOT PMP entry for ROM write protection
#define PMP_ENTRY_ROM_WP_NAPOT       14
// NAPOT PMP entry for FLASH write protection
#define PMP_ENTRY_FLASH_WP_NAPOT     15
// NAPOT PMP entry for U-mode global permissions.
#define PMP_ENTRY_USER_GLOBAL_NAPOT  11

// Kernel supports virtual memory.
#define MEMMAP_VMEM             0
// Page size for memory protections.
#define MEMMAP_PAGE_SIZE        4096
// Maximum number of mapped regions per process.
#define PROC_MEMMAP_MAX_REGIONS 8
// Lowest numbered PMP to use for process memory maps, must be a multiple of 4.
#define PROC_RISCV_PMP_START    0
// Number of PMPs to use for process memory maps, must be a multiple of 4.
#define PROC_RISCV_PMP_COUNT    8
