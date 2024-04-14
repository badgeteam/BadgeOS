
// SPDX-License-Identifier: MIT

#pragma once

#include "scheduler/scheduler.h"



// Process is running or waiting for syscalls.
#define PROC_RUNNING 0x00000001
// Process is waiting for threads to exit.
#define PROC_EXITING 0x00000002
// Process has fully exited.
#define PROC_EXITED  0x00000004



// A process and all of its resources.
typedef struct process_t process_t;
// Globally unique process ID.
typedef int              pid_t;

// Create a new, empty process.
pid_t    proc_create(badge_err_t *ec);
// Delete a process and release any resources it had.
void     proc_delete(pid_t pid);
// Get the process' flags.
uint32_t proc_getflags(badge_err_t *ec, pid_t pid);

// Set arguments for a process.
// If omitted, argc will be 0 and argv will be NULL.
void proc_setargs(badge_err_t *ec, pid_t pid, int argc, char const *const *argv);
// Load an executable and start a prepared process.
void proc_start(badge_err_t *ec, pid_t pid, char const *executable);

// Allocate more memory to a process.
// Returns actual virtual address on success, 0 on failure.
size_t proc_map(badge_err_t *ec, pid_t pid, size_t vaddr_req, size_t min_size, size_t min_align, int flags);
// Release memory allocated to a process.
void   proc_unmap(badge_err_t *ec, pid_t pid, size_t base);
// Whether the process owns this range of memory.
// Returns the lowest common denominator of the access bits bitwise or 8.
int    proc_map_contains(badge_err_t *ec, pid_t pid, size_t base, size_t size);
