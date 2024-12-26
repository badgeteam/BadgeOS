
// SPDX-License-Identifier: MIT

#include "assertions.h"
#include "cpu/riscv_sbi.h"
#include "interrupt.h"
#include "port/dtb.h"
#include "port/hardware_allocation.h"
#include "scheduler/isr.h"
#include "time.h"
#include "time_private.h"



// Whether the SBI time extension is supported.
static bool     support_sbi_time;
// Ticks per second.
static uint32_t ticks_per_sec;
// Tick offset for the purpose of timekeeping.
static uint64_t base_tick;


// Get the current time in ticks.
static inline uint64_t time_ticks() {
#if __riscv_xlen == 32
    uint32_t ticks_lo0, ticks_lo1;
    uint32_t ticks_hi0, ticks_hi1;
    asm("rdtimeh %0; rdtime %1" : "=r"(ticks_hi0), "=r"(ticks_lo0));
    asm("rdtimeh %0; rdtime %1" : "=r"(ticks_hi1), "=r"(ticks_lo1));
    uint64_t ticks;
    if (ticks_hi0 != ticks_hi1) {
        ticks = ((uint64_t)ticks_hi1 << 32) | ticks_lo1;
    } else {
        ticks = ((uint64_t)ticks_hi0 << 32) | ticks_lo0;
    }
#else
    uint64_t ticks;
    asm("rdtime %0" : "=r"(ticks));
#endif
    return ticks - base_tick;
}

// Set the timer for a certain timestamp.
static void set_timer_ticks(uint64_t timestamp) {
    if (support_sbi_time) {
        sbi_set_timer(timestamp + base_tick);
    } else {
        sbi_legacy_set_timer(timestamp + base_tick);
    }
}

// Set the CPU's timer to a certain timestamp.
void time_set_cpu_timer(timestamp_us_t timestamp) {
    set_timer_ticks(timestamp * ticks_per_sec / 1000000);
    asm("csrs sie, %0" ::"r"(1 << RISCV_INT_SUPERVISOR_TIMER));
}

// Clear the CPU's timer.
void time_clear_cpu_timer() {
    asm("csrc sie, %0" ::"r"(1 << RISCV_INT_SUPERVISOR_TIMER));
}


// Timer init code common to DTB and ACPI.
static void time_init_common() {
    // Test for SBI time extension.
    sbi_ret_t res    = sbi_probe_extension(SBI_TIME_EID);
    support_sbi_time = res.retval != 0;
    // Set base tick to now so that time_us returns micros since boot.
    base_tick        = time_ticks();
    if (support_sbi_time) {
        logk(LOG_INFO, "Using SBI timer");
    } else {
        logk(LOG_INFO, "Using legacy SBI timer");
    }
    // Finally, run generic timer init code.
    time_init_generic();
}

// Initialise timer and watchdog subsystem.
void time_init_dtb(dtb_handle_t *handle) {
    dtb_node_t *cpus = dtb_get_node(handle, dtb_root_node(handle), "cpus");
    if (!cpus) {
        logkf(LOG_FATAL, "DTB missing `cpus` node");
        panic_poweroff();
    }
    dtb_prop_t *timebase_freq = dtb_get_prop(handle, cpus, "timebase-frequency");
    if (!timebase_freq) {
        logkf(LOG_FATAL, "DTB node `cpus` missing prop `timebase-frequency`");
        panic_poweroff();
    }
    if (timebase_freq->content_len != 4) {
        logkf(
            LOG_FATAL,
            "DTB node `cpus` prop `timebase-frequency` has invalid length (expected 4, get %{u32;d})",
            timebase_freq->content_len
        );
        panic_poweroff();
    }
    ticks_per_sec = dtb_prop_read_uint(handle, timebase_freq);
    time_init_common();
}

// Get current time in microseconds.
timestamp_us_t time_us() {
    if (!ticks_per_sec) {
        return 0;
    }
    return time_ticks() * 1000000 / ticks_per_sec;
}

// Called by the interrupt handler when the CPU-local timer fires.
void riscv_sbi_timer_interrupt() {
    time_cpu_timer_isr();
}
