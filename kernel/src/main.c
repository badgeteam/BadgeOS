
// SPDX-License-Identifier: MIT

#include "assertions.h"
#include "badge_err.h"
#include "filesystem.h"
#include "housekeeping.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "log.h"
#include "malloc.h"
#include "memprotect.h"
#include "port/port.h"
#include "process/process.h"
#include "scheduler/scheduler.h"
#include "time.h"

#include <stdatomic.h>

#include <config.h>



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
    logk(LOG_INFO, "BadgerOS " CONFIG_TARGET " starting...");

    // Memory protection initialization.
    memprotect_init();
    // Kernel memory allocator initialization.
    kernel_heap_init();

    // Global scheduler initialization.
    sched_init();

    // Housekeeping thread initialization.
    hk_init();
    // Add the remainder of the kernel lifetime as a new thread.
    tid_t thread = thread_new_kernel(&ec, "main", (void *)kernel_lifetime_func, NULL, SCHED_PRIO_NORMAL);
    badge_err_assert_always(&ec);
    thread_resume(&ec, thread);
    thread_resume(&ec, thread);
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
    logk(LOG_INFO, "Starting init process");

    char const *initbin = "/sbin/init";
    pid_t       pid     = proc_create(&ec, -1, "/sbin/init", 1, &initbin);
    badge_err_assert_always(&ec);
    assert_dev_drop(pid == 1);
    proc_start(&ec, pid);
    badge_err_assert_always(&ec);
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
