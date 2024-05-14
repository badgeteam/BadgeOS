
// SPDX-License-Identifier: MIT

#include "process/syscall_impl.h"

#include "process/internal.h"
#include "process/sighandler.h"
#include "process/types.h"
#include "scheduler/cpu.h"
#include "signal.h"

// Sycall: Exit the process; exit code can be read by parent process.
// When this system call returns, the thread will be suspended awaiting process termination.
void syscall_proc_exit(int code) {
    proc_exit_self(code);
}



void *syscall_mem_alloc(size_t vaddr_req, size_t min_size, size_t min_align, int flags) {
    return (void *)proc_map_raw(NULL, proc_current(), vaddr_req, min_size, min_align, flags);
}

size_t syscall_mem_size(void *address) {
    process_t *const proc = proc_current();
    mutex_acquire_shared(NULL, &proc->mtx, TIMESTAMP_US_MAX);
    size_t res = 0;
    for (size_t i = 0; i < proc->memmap.regions_len; i++) {
        if (proc->memmap.regions[i].base == (size_t)address) {
            res = proc->memmap.regions[i].size;
            break;
        }
    }
    mutex_release_shared(NULL, &proc->mtx);
    return res;
}

bool syscall_mem_dealloc(void *address) {
    badge_err_t *ec = {0};
    proc_unmap_raw(ec, proc_current(), (size_t)address);
    return badge_err_is_ok(ec);
}

void *syscall_proc_sighandler(int signum, void *newhandler) {
    if (signum < 0 || signum >= SIG_COUNT) {
        proc_sigsys_handler();
    }
    process_t *const proc = proc_current();
    mutex_acquire(NULL, &proc->mtx, TIMESTAMP_US_MAX);
    void *old                 = (void *)proc->sighandlers[signum];
    proc->sighandlers[signum] = (size_t)newhandler;
    mutex_release(NULL, &proc->mtx);
    return old;
}

void syscall_proc_sigret() {
    sched_signal_exit();
}
