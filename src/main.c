
// SPDX-License-Identifier: MIT

#include "assertions.h"
#include "filesystem.h"
#include "filesystem/vfs_internal.h"
#include "filesystem/vfs_ramfs.h"
#include "gpio.h"
#include "log.h"
#include "malloc.h"
#include "memprotect.h"
#include "port/interrupt.h"
#include "rawprint.h"
#include "scheduler.h"
#include "time.h"

#include <stdint.h>

// Temporary kernel context until threading is implemented.
static kernel_ctx_t kctx;

void debug_func(void *);
#define stack_size 8192
static uint8_t stack0[stack_size] ALIGNED_TO(STACK_ALIGNMENT);

static inline void check_ec(badge_err_t *ec) {
    if (ec && !badge_err_is_ok(ec)) {
        logkf(LOG_ERROR, "ELOC=%{d}, ECAUSE=%{d}", ec->location, ec->cause);
        sched_destroy_thread(NULL, sched_get_current_thread());
    }
}

// This is the entrypoint after the stack has been set up and the init functions
// have been run. Main is not allowed to return, so declare it noreturn.
void main() NORETURN;
void main() {
    badge_err_t err;

    // Install interrupt and trap handlers.
    interrupt_init(&kctx);

    // Set up memory protection.
    memprotect_init();

    // Set up timers and watchdogs.
    // This function must run within the first ~1s of power-on time and should be
    // called as early as possible.
    time_init();

    // Set up memory allocator.
    kernel_heap_init();

    // Set up multithreading.
    sched_init(&err);
    assert_always(badge_err_is_ok(&err));

    // A debug thread.
    sched_thread_t *const debug_thread_0 =
        sched_create_kernel_thread(&err, debug_func, NULL, stack0, stack_size, SCHED_PRIO_NORMAL);
    assert_always(badge_err_is_ok(&err));
    sched_set_name(&err, debug_thread_0, "debug0");
    assert_always(badge_err_is_ok(&err));
    sched_resume_thread(&err, debug_thread_0);
    assert_always(badge_err_is_ok(&err));

    // Enter the scheduler
    sched_exec();
    __builtin_unreachable();
}

void debug_func(void *arg) {
    (void)arg;
    badge_err_t ec;

    // Create RAMFS.
    logk(LOG_DEBUG, "Creating RAMFS at /");
    fs_mount(&ec, FS_TYPE_RAMFS, NULL, "/", 0);
    check_ec(&ec);

    // Create a directory.
    logk(LOG_DEBUG, "Creating a directory at /foo");
    file_t dirfd = fs_dir_open(&ec, "/foo", OFLAGS_CREATE);
    check_ec(&ec);

    // Create a subdirectory.
    logk(LOG_DEBUG, "Creating a directory at /foo/bar");
    fs_dir_create(&ec, "/foo/bar");
    check_ec(&ec);

    // Create a file.
    logk(LOG_DEBUG, "Opening a file at /foo/a.txt");
    /* Walk didn't do it's job and opened '/' instead of '/foo' to create 'a.txt' in. */
    file_t fd = fs_open(&ec, "/foo/a.txt", OFLAGS_CREATE | OFLAGS_READWRITE);
    check_ec(&ec);

    // Write some data to it.
    logk(LOG_DEBUG, "Writing data to file");
    fs_write(&ec, fd, "Hi.", 3);
    check_ec(&ec);

    // Seek to start.
    logk(LOG_DEBUG, "Seeking to 0");
    fs_seek(&ec, fd, 0, SEEK_ABS);
    check_ec(&ec);

    // Read some data from it.
    logk(LOG_DEBUG, "Reading data from file");
    char      readbuf[4];
    fileoff_t len = fs_read(&ec, fd, readbuf, 3);
    check_ec(&ec);
    logk_hexdump_vaddr(LOG_DEBUG, "Read data:", readbuf, 3, 0);

    // List the directory.
    dirent_t ent;
    while (fs_dir_read(&ec, &ent, dirfd)) {
        logkf(LOG_DEBUG, "Inode: %{d}, Is dir: %{d}, Name: %{cs}", ent.inode, ent.is_dir, ent.name);
    }



    // (void)arg;
    // io_mode(NULL, 19, IO_MODE_OUTPUT);
    // timestamp_us_t t;
    // while (1) {
    //     io_write(NULL, 19, true);
    //     t = time_us() + 500000;
    //     while (time_us() < t) sched_yield();
    //     io_write(NULL, 19, false);
    //     t = time_us() + 500000;
    //     while (time_us() < t) sched_yield();
    // }
}
