
// SPDX-License-Identifier: MIT

#pragma once

#include "attributes.h"
#include "badge_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct process_t process_t;

// Globally unique thread ID.
typedef int tid_t;

typedef struct sched_thread_t sched_thread_t;

typedef void (*sched_entry_point_t)(void *arg);

typedef enum {
    // will be scheduled with smaller time slices than normal
    SCHED_PRIO_LOW    = 0,
    // default value
    SCHED_PRIO_NORMAL = 10,
    // will be scheduled with bigger time slices than normal
    SCHED_PRIO_HIGH   = 20,
} sched_prio_t;

// Initializes the scheduler and setups up the system to be ready to
// create threads and execute them.
void sched_init(badge_err_t *ec);

// Kicks of the scheduler and runs till the system end.
// This will never return.
void sched_exec(void) NORETURN;

// Creates a new suspended userland thread.
//
// Userland threads have no initial stack pointer set, this must be done by
// `entry_point` in the userland application itself.
//
// - `process` is the process that is associated with this thread.
// - `entry_point` is the function the thread will execute.
// - `arg` is passed to `entry_point` upon start.
// - `priority` defines how much time the thread gets in regards to all other
// threads. Higher priority threads will have
//   a higher time contigent as others.
//
// Returns a handle to the thread or NULL if the thread could not be created.
//
// Potential errors:
// - `ECAUSE_NOMEM` is issued when the thread could not be allocated.
sched_thread_t *sched_create_userland_thread(
    badge_err_t        *ec,
    process_t          *process,
    sched_entry_point_t entry_point,
    void               *arg,
    void               *stack_bottom,
    size_t              stack_size,
    sched_prio_t        priority
);

// Creates a new suspended kernel thread.
//
// - `process` is the process that is associated with this thread.
// - `entry_point` is the function the thread will execute.
// - `arg` is passed to `entry_point` upon start.
// - `stack_bottom` is a pointer to the stack space for this thread. Pointer
// must be aligned to `STACK_ALIGNMENT` and
//   must not be `NULL`.
// - `stack_size` is the number of bytes available from `stack_bottom` on. Must
// be a multiple of `STACK_ALIGNMENT`.
// - `priority` defines how much time the thread gets in regards to all other
// threads. Higher priority threads will have
//   a higher time contigent as others.
//
// Returns a handle to the thread or NULL if the thread could not be created.
//
// Potential errors:
// - `ECAUSE_NOMEM` is issued when the thread could not be allocated.
sched_thread_t *sched_create_kernel_thread(
    badge_err_t        *ec,
    sched_entry_point_t entry_point,
    void               *arg,
    void               *stack_bottom,
    size_t              stack_size,
    sched_prio_t        priority
);

// Kills the given thread and releases all scheduler resources allocated by the
// operating system associated with this thread.
//
// NOTE: If `thread` is the current thread, this function is equivalent of
//       detaching the thread and then calling `sched_exit(0)`.
//
// NOTE: This does only destroys scheduler-related resources, but not other
// kernel resources.
void sched_destroy_thread(badge_err_t *ec, sched_thread_t *thread);

// Detaches the thread. This means that when the thread stops by returning from
// `entry_point`, the thread is automatically destroyed.
void sched_detach_thread(badge_err_t *ec, sched_thread_t *thread);

// Halts the thread and prevents it from being scheduled again.
// This effect can be undone with `sched_resume_thread`.
//
// If the thread is already suspended, nothing will happen.
//
// If `thread` is NULL, the current thread will be suspended and `sched_yield()`
// is invoked implicitly. It will return with success after the thread will be
// resumed again.
//
// Potential errors:
// - `ECAUSE_ILLEGAL` is issued when the thread has finished.
void sched_suspend_thread(badge_err_t *ec, sched_thread_t *thread);

// Resumes a previously suspended thread or starts it.
// After that, the thread will be scheduled in a regular manner again.
//
// If the thread is already running, nothing will happen.
//
// Potential errors:
// - `ECAUSE_ILLEGAL` is issued when the thread has finished.
void sched_resume_thread(badge_err_t *ec, sched_thread_t *thread);

// Resumes a previously suspended thread or starts it.
// Will move the thread to the start of the wait queue and will make it the
// next thread executed.
//
// NOTE: When `thread` is the current thread, this function will do nothing.
//
// Potential errors:
// - `ECAUSE_ILLEGAL` is issued when the thread has finished.
void sched_resume_thread_next(badge_err_t *ec, sched_thread_t *thread);

// Returns whether a thread is running; it is neither suspended nor has it exited.
bool sched_thread_is_running(badge_err_t *ec, sched_thread_t *thread);

// Returns the currently active thread or NULL if the scheduler isn't running.
sched_thread_t *sched_get_current_thread(void);

// Returns the associated process for a given thread.
process_t *sched_get_associated_process(sched_thread_t const *thread);

// thread self-control interface:

// Announces that all work is done for now and the scheduler can now
// schedule other threads.
//
// NOTE: It's illegal to invoke this function outside a thread context!
void sched_yield(void);

// Exits the current thread and notifies the scheduler that it must not be
// scheduled anymore, as all work is done.
//
// NOTE: It's illegal to invoke this function outside a thread context!
void sched_exit(uint32_t exit_code) NORETURN;


// Debug: Set thread name shown in logs.
void sched_set_name(badge_err_t *ec, sched_thread_t *thread, char const *name);

// Debug: Get thread name shown in logs.
char const *sched_get_name(sched_thread_t *thread);
