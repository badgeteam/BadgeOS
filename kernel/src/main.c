
// SPDX-License-Identifier: MIT

#include "backtrace.h"
#include "badge_err.h"
#include "esp_intmtx.h"
#include "filesystem.h"
#include "hal/gpio.h"
#include "hal/i2c.h"
#include "hal/spi.h"
#include "housekeeping.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "log.h"
#include "malloc.h"
#include "memprotect.h"
#include "port/port.h"
#include "process/process.h"
#include "scheduler/scheduler.h"
#include "soc/timer_group_struct.h"
#include "time.h"

#include <stdatomic.h>



// The initial kernel stack.
extern char          stack_bottom[] asm("__stack_bottom");
extern char          stack_top[] asm("__stack_top");
// When set, a shutdown is initiated.
// 0: Do nothing.
// 1: Shut down (default).
// 2: Reboot.
atomic_int           kernel_shutdown_mode;
// Temporary file image.
extern uint8_t const elf_rom[];
extern size_t const  elf_rom_len;


#define show_csr(name)                                                                                                 \
    do {                                                                                                               \
        long csr;                                                                                                      \
        asm("csrr %0, " #name : "=r"(csr));                                                                            \
        logkf(LOG_INFO, #name ": %{long;x}", csr);                                                                     \
    } while (0)

extern void init_ramfs();
static void kernel_init();
static void userland_init();
// static void userland_shutdown();
// static void kernel_shutdown();

// Manages the kernel's lifetime after basic runtime initialization.
static void kernel_lifetime_func() {
    // Start the kernel services.
    kernel_init();
    // TODO: Start other CPUs.
    // cpu_multicore_init();
    // Start userland.
    userland_init();



    // The boot process is now complete, this thread will wait until a shutdown is issued.
    int shutdown_mode;
    do {
        sched_yield();
        shutdown_mode = atomic_load(&kernel_shutdown_mode);
    } while (shutdown_mode == 0);

    // TODO: Shutdown process.
    logk(LOG_INFO, "TODO: Shutdown procedure.");
    while (1) continue;
}

// Shutdown system call implementation.
void syscall_sys_shutdown(bool is_reboot) {
    logk(LOG_INFO, is_reboot ? "Reboot requested" : "Shutdown requested");
    atomic_store(&kernel_shutdown_mode, 1 + is_reboot);
}



static void test_isr() {
    logk_from_isr(LOG_DEBUG, "Timer interrupt!");
    timer_int_enable(1, false);
    timer_stop(1);
}
extern intmtx_t INTMTX0;

#define DUMP_CSR(name)                                                                                                 \
    {                                                                                                                  \
        long tmp;                                                                                                      \
        asm("csrr %0, " #name : "=r"(tmp));                                                                            \
        logkf(LOG_DEBUG, "CSR " #name ": %{long;x}", tmp);                                                             \
    }

// After control handover, the booting CPU core starts here and other cores wait.
// This sets up the basics of everything needed by the other systems of the kernel.
// When finished, the booting CPU will perform kernel initialization.
void basic_runtime_init() {
    badge_err_t ec = {0};

    // ISR initialization.
    irq_init();
    // Early platform initialization.
    port_early_init();

    // Timekeeping initialization.
    time_init();

    // Announce that we're alive.
    logk(LOG_INFO, "BadgerOS starting...");

    // Kernel memory allocator initialization.
    kernel_heap_init();
    // Memory protection initialization.
    memprotect_init();

    // Timer ISR test.
    irq_ch_route(ETS_TG1_T0_INTR_SOURCE, 29);
    irq_ch_set_isr(29, test_isr);
    irq_ch_enable(29, true);
    timer_set_freq(1, 1000000);
    timer_value_set(1, 0);
    timer_start(1);
    timer_alarm_config(1, 500000, false);
    timer_int_enable(1, true);
    irq_enable(true);
    asm("csrs mie, %0" ::"r"(0xffffffff));
    while (timer_value_get(1) < 500000);
    logk(LOG_DEBUG, "Interrupt should have fired");
    logkf(
        LOG_DEBUG,
        "Pending: %{u32;x} %{u32;x} %{u32;x} %{u32;x}",
        INTMTX0.pending[0],
        INTMTX0.pending[1],
        INTMTX0.pending[2],
        INTMTX0.pending[3]
    );
    logkf(LOG_DEBUG, "TG0 raw: %{u32;x}", TIMERG0.int_raw_timers);
    logkf(LOG_DEBUG, "TG1 raw: %{u32;x}", TIMERG1.int_raw_timers);
    logkf(LOG_DEBUG, "TG0 st: %{u32;x}", TIMERG0.int_st_timers);
    logkf(LOG_DEBUG, "TG1 st: %{u32;x}", TIMERG1.int_st_timers);
    logkf(LOG_DEBUG, "TG0 T0 IRQ: %{d}", (INTMTX0.pending[1] >> (ETS_TG0_T0_INTR_SOURCE - 32)) & 1);
    logkf(LOG_DEBUG, "TG0 T1 IRQ: %{d}", (INTMTX0.pending[1] >> (ETS_TG0_T1_INTR_SOURCE - 32)) & 1);
    logkf(LOG_DEBUG, "TG1 T0 IRQ: %{d}", (INTMTX0.pending[1] >> (ETS_TG1_T0_INTR_SOURCE - 32)) & 1);
    logkf(LOG_DEBUG, "TG1 T1 IRQ: %{d}", (INTMTX0.pending[1] >> (ETS_TG1_T1_INTR_SOURCE - 32)) & 1);
    logkf(LOG_DEBUG, "Pending: %{d}", irq_ch_pending(29));
    DUMP_CSR(mstatus)
    DUMP_CSR(mip)
    DUMP_CSR(mie)
    DUMP_CSR(mtvec)

    while (1);

    // Scheduler initialization.
    sched_init();
    // Housekeeping thread initialization.
    hk_init();
    // Add the remainder of the kernel lifetime as a new thread.
    sched_thread_t *thread = sched_create_kernel_thread(
        &ec,
        kernel_lifetime_func,
        NULL,
        stack_bottom,
        stack_top - stack_bottom,
        SCHED_PRIO_NORMAL
    );
    badge_err_assert_always(&ec);
    sched_resume_thread(&ec, thread);
    badge_err_assert_always(&ec);

    // Start the scheduler and enter the next phase in the kernel's lifetime.
    sched_exec();
}



// After basic runtime initialization, the booting CPU core continues here.
// This finishes the initialization of all kernel systems, resources and services.
// When finished, the non-booting CPUs will be started (method and entrypoints to be determined).
static void kernel_init() {
    badge_err_t ec = {0};
    // Full hardware initialization.
    port_init();

    // Temporary filesystem image.
    fs_mount(&ec, FS_TYPE_RAMFS, NULL, "/", 0);
    badge_err_assert_always(&ec);
    init_ramfs();
}



// After kernel initialization, the booting CPU core continues here.
// This starts up the `init` process while other CPU cores wait for processes to be scheduled for them.
// When finished, this function returns and the thread should wait for a shutdown event.
static void userland_init() {
    badge_err_t ec = {0};
    logk(LOG_INFO, "Kernel initialized");
    logk(LOG_INFO, "Staring init process");

    pid_t pid = proc_create(&ec);
    badge_err_assert_always(&ec);
    assert_dev_drop(pid == 1);
    proc_start(&ec, pid, "/sbin/init");
    badge_err_assert_always(&ec);
    while (1) continue;
}



// // When a shutdown event begins, exactly one CPU core runs this entire function.
// // This signals all processes to exit (or be killed if they wait too long) and shuts down other CPU cores.
// // When finished, the CPU continues to shut down the kernel.
// static void userland_shutdown() {
// }



// // When the userspace has been shut down, the CPU continues here.
// // This will synchronize all filesystems and clean up any other resources not needed to finish hardware shutdown.
// // When finished, the CPU continues to the platform-specific hardware shutdown / reboot handler.
// static void kernel_shutdown() {
// }
