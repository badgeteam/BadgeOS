
// SPDX-License-Identifier: MIT

#include "process/syscall_impl.h"

#include "process/internal.h"
#include "process/process.h"
#include "process/types.h"
#include "scheduler/types.h"

// Sycall: Exit the process; exit code can be read by parent process.
// When this system call returns, the thread will be suspended awaiting process termination.
void syscall_self_exit(int code) {
    // Mark this process as exiting.
    sched_thread_t *thread  = sched_get_current_thread();
    process_t      *process = thread->process;
    assert_always(mutex_acquire(NULL, &process->mtx, PROC_MTX_TIMEOUT));
    process->flags |= PROC_EXITING;
    mutex_release(NULL, &process->mtx);

    // TODO: Add deleting runtime to the housekeeping list.
}
