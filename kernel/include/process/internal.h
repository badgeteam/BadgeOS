
// SPDX-License-Identifier: MIT

#pragma once

#include "filesystem.h"
#include "process/process.h"

// Kill a process from one of its own threads.
void proc_exit_self(int code);

// Look up a process without locking the global process mutex.
process_t *proc_get_unsafe(pid_t pid);

// Suspend all threads for a process except the current.
void proc_suspend(process_t *process, sched_thread_t *current);
// Resume all threads for a process.
void proc_resume(process_t *process);
// Release all process runtime resources (threads, memory, files, etc.).
// Does not remove args, exit code, etc.
void proc_delete_runtime_raw(process_t *process);

// Create a new, empty process.
process_t *proc_create_raw(badge_err_t *ec, pid_t parent, char const *binary, int argc, char const *const *argv);
// Get a process handle by ID.
process_t *proc_get(pid_t pid);
// Get the process' flags.
uint32_t   proc_getflags_raw(process_t *process);
// Get a handle to the current process, if any.
process_t *proc_current();

// Load an executable and start a prepared process.
void proc_start_raw(badge_err_t *ec, process_t *process);

// Create a new thread in a process.
// Returns created thread handle.
sched_thread_t *proc_create_thread_raw_unsafe(
    badge_err_t *ec, process_t *process, sched_entry_point_t entry_point, void *arg, sched_prio_t priority
);
// Delete a thread in a process.
void   proc_delete_thread_raw_unsafe(badge_err_t *ec, process_t *process, sched_thread_t *thread);
// Allocate more memory to a process.
// Returns actual virtual address on success, 0 on failure.
size_t proc_map_raw(badge_err_t *ec, process_t *process, size_t vaddr, size_t size, size_t align, int flags);
// Release memory allocated to a process.
void   proc_unmap_raw(badge_err_t *ec, process_t *process, size_t base);
// Whether the process owns this range of memory.
// Returns the lowest common denominator of the access bits bitwise or 8.
int    proc_map_contains_raw(process_t *proc, size_t base, size_t size);
// Add a file to the process file handle list.
int    proc_add_fd_raw(badge_err_t *ec, process_t *process, file_t real);
// Find a file in the process file handle list.
file_t proc_find_fd_raw(badge_err_t *ec, process_t *process, int virt);
// Remove a file from the process file handle list.
void   proc_remove_fd_raw(badge_err_t *ec, process_t *process, int virt);

// Perform a pre-resume check for a user thread.
// Used to implement asynchronous events.
void proc_pre_resume_cb(sched_thread_t *thread);
// Atomically whether signals are pending.
bool proc_signals_pending_raw(process_t *process);
// Raise a signal to a process' main thread or a specified thread, while suspending it's other threads.
void proc_raise_signal_raw(badge_err_t *ec, process_t *process, int signum);
