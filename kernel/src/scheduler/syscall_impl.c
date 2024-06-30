
// SPDX-License-Identifier: MIT

#include "scheduler/scheduler.h"
#include "syscall.h"

void syscall_thread_yield() {
    sched_yield();
}
