
// SPDX-License-Identifier: MIT

#pragma once

#include "scheduler/scheduler.h"



// Process is running or waiting for syscalls.
#define PROC_RUNNING  0x00000001
// Process is waiting for threads to exit.
#define PROC_EXITING  0x00000002
// Process has fully exited.
#define PROC_EXITED   0x00000004
// Process has signals pending.
#define PROC_SIGPEND  0x00000008
// Process is pre-start.
#define PROC_PRESTART 0x00000010
// Process has a state change not acknowledged.
#define PROC_STATECHG 0x00000020



// A process and all of its resources.
typedef struct process_t process_t;
// Globally unique process ID.
typedef int              pid_t;

// Send a signal to all running processes in the system except the init process.
void proc_signal_all(int signal);
// Whether any non-init processes are currently running.
bool proc_has_noninit();

// Create a new, empty process.
pid_t    proc_create(badge_err_t *ec, pid_t parent, char const *binary, int argc, char const *const *argv);
// Delete a process only if it hasn't been started yet.
bool     proc_delete_prestart(pid_t pid);
// Delete a process and release any resources it had.
void     proc_delete(pid_t pid);
// Get the process' flags.
uint32_t proc_getflags(badge_err_t *ec, pid_t pid);
// Get the PID of the current process, if any.
pid_t    proc_current_pid();
// Load an executable and start a prepared process.
void     proc_start(badge_err_t *ec, pid_t pid);

// Allocate more memory to a process.
// Returns actual virtual address on success, 0 on failure.
size_t proc_map(badge_err_t *ec, pid_t pid, size_t vaddr_req, size_t min_size, size_t min_align, int flags);
// Release memory allocated to a process.
void   proc_unmap(badge_err_t *ec, pid_t pid, size_t base);
// Whether the process owns this range of memory.
// Returns the lowest common denominator of the access bits bitwise or 8.
int    proc_map_contains(badge_err_t *ec, pid_t pid, size_t base, size_t size);

// Raise a signal to a process, which may be the current process.
void proc_raise_signal(badge_err_t *ec, pid_t pid, int signum);

// Check whether a process is a parent to another.
bool proc_is_parent(pid_t parent, pid_t child);
