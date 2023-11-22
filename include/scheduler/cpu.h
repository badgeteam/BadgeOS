
// SPDX-License-Identifier: MIT

#pragma once

#include "attributes.h"
#include "badge_err.h"
#include "scheduler/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Requests the scheduler to prepare a switch from userland to kernel for a user thread.
// If `syscall` is true, copies registers `a0` through `a7` to the kernel thread.
// The kernel thread will start at `entry_point`.
void sched_raise_from_isr(bool syscall, void *entry_point);

// Requests the scheduler to prepare a switch from kernel to userland for a user thread.
// Resumes the userland thread where it left off.
void sched_lower_from_isr();

// Prepares a context to be invoked as a kernel thread.
void sched_prepare_kernel_entry(sched_thread_t *thread, sched_entry_point_t entry_point, void *arg);

// Prepares a pair of contexts to be invoked as a userland thread.
// Kernel-side in these threads is always started by an ISR and the entry point is given at that time.
void sched_prepare_user_entry(sched_thread_t *thread, sched_entry_point_t entry_point, void *arg);
