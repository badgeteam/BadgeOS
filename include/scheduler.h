
// SPDX-License-Identifier: MIT

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <badge_err.h>

typedef struct process_t process_t;

typedef struct sched_thread_t sched_thread_t;

typedef void (*sched_entry_point_t)(void *arg);

typedef enum
{
    // will be scheduled with smaller time slices than normal
    SCHED_PRIO_LOW = 0,
    // default value
    SCHED_PRIO_NORMAL = 10,
    // will be scheduled with bigger time slices than normal
    SCHED_PRIO_HIGH = 20,
} sched_thread_priority_t;

// Initializes the scheduler and setups up the system to be ready to
// create threads and execute them.
void sched_init(badge_err_t *ec);

// Kicks of the scheduler and runs until no threads are active anymore.
// Returns as soon as all threads are stopped or suspended.
// This also includes kernel threads.
void sched_run(void);

// Creates a new suspended userland thread.
//
// Userland threads have no initial stack pointer set, this must be done by `entry_point` in the userland application
// itself.
//
// - `process` is the process that is associated with this thread.
// - `entry_point` is the function the thread will execute.
// - `arg` is passed to `entry_point` upon start.
// - `priority` defines how much time the thread gets in regards to all other threads. Higher priority threads will have
//   a higher time contigent as others.
//
// Returns a handle to the thread or NULL if the thread could not be created.
sched_thread_t *sched_create_userland_thread(badge_err_t *ec, process_t *process, sched_entry_point_t entry_point,
                                             void *arg, sched_thread_priority_t priority);

// Creates a new suspended kernel thread.
//
// - `process` is the process that is associated with this thread.
// - `entry_point` is the function the thread will execute.
// - `arg` is passed to `entry_point` upon start.
// - `stack_bottom` is a pointer to the stack space for this thread. Pointer must be aligned to `STACK_ALIGNMENT`
// and
//   must not be `NULL`.
// - `stack_size` is the number of bytes available from `stack_bottom` on. Must be a multiple of `STACK_ALIGNMENT`.
// - `priority` defines how much time the thread gets in regards to all other threads. Higher priority threads will
// have
//   a higher time contigent as others.
//
// Returns a handle to the thread or NULL if the thread could not be created.
sched_thread_t *sched_create_kernel_thread(badge_err_t *ec, sched_entry_point_t entry_point, void *arg,
                                           void *stack_bottom, size_t stack_size, sched_thread_priority_t priority);

// Kills the given thread and releases all scheduler resources allocated by the operating system associated with this
// thread.
//
// `thread` can be `NULL` to signal that the currently executing thread should be killed.
//
// NOTE: This does only include scheduler-related resources, but not other kernel resources.
void sched_destroy_thread(badge_err_t *ec, sched_thread_t *thread);

// Detaches the thread. This means that when the thread stops by returning from `entry_point`,
// the thread is automatically destroyed.
void sched_detach_thread(badge_err_t *ec, sched_thread_t *thread);

// Halts the thread and prevents it from being scheduled again.
// This effect can be undone with `sched_resume_thread`.
// If `thread` is NULL, the current thread will be suspended and `sched_yield()` is invoked implicitly.
void sched_suspend_thread(badge_err_t *ec, sched_thread_t *thread);

// Resumes a previously suspended thread or starts it.
// After that, the thread will be scheduled in a regular manner again.
void sched_resume_thread(badge_err_t *ec, sched_thread_t *thread);

// Announces that all work is done for now and the scheduler can now
// schedule other threads.
//
// NOTE: It's illegal to invoke this function outside a thread context!
void sched_yield(void);

// Returns the currently active thread or NULL if the scheduler isn't running.
sched_thread_t *sched_get_current_thread(void);

// Returns the associated process for a given thread.
process_t *sched_get_associated_process(sched_thread_t const *thread);
