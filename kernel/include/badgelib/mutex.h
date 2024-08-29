
// SPDX-License-Identifier: MIT

#pragma once

#include "list.h"
#include "time.h"

#include <stdatomic.h>
#include <stdbool.h>

typedef struct {
    // Mutex allows sharing.
    bool        is_shared;
    // Allow accessing from ISR.
    bool        allow_isr;
    // Spinlock guarding the waiting list.
    atomic_flag wait_spinlock;
    // Share count and/or is locked.
    atomic_int  shares;
    // List of threads waiting for this mutex.
    dlist_t     waiting_list;
} mutex_t;

#define MUTEX_T_INIT            ((mutex_t){0, 0, ATOMIC_FLAG_INIT, 0, {0}})
#define MUTEX_T_INIT_SHARED     ((mutex_t){1, 0, ATOMIC_FLAG_INIT, 0, {0}})
#define MUTEX_T_INIT_ISR        ((mutex_t){0, 1, ATOMIC_FLAG_INIT, 0, {0}})
#define MUTEX_T_INIT_SHARED_ISR ((mutex_t){1, 1, ATOMIC_FLAG_INIT, 0, {0}})

#include "badge_err.h"



// Recommended way to create a mutex at run-time.
void mutex_init(badge_err_t *ec, mutex_t *mutex, bool shared, bool allow_isr);
// Clean up the mutex.
void mutex_destroy(badge_err_t *ec, mutex_t *mutex);

// Try to acquire `mutex` within `max_wait_us` microseconds.
// If `max_wait_us` is too long or negative, do not use the timeout.
// Returns true if the mutex was successully acquired.
bool mutex_acquire(badge_err_t *ec, mutex_t *mutex, timestamp_us_t max_wait_us);
// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release(badge_err_t *ec, mutex_t *mutex);
// Try to acquire `mutex` within `max_wait_us` microseconds.
// If `max_wait_us` is too long or negative, do not use the timeout.
// Returns true if the mutex was successully acquired.
bool mutex_acquire_from_isr(badge_err_t *ec, mutex_t *mutex, timestamp_us_t max_wait_us);
// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release_from_isr(badge_err_t *ec, mutex_t *mutex);

// Try to acquire a share in `mutex` within `max_wait_us` microseconds.
// If `max_wait_us` is too long or negative, do not use the timeout.
// Returns true if the share was successfully acquired.
bool mutex_acquire_shared(badge_err_t *ec, mutex_t *mutex, timestamp_us_t max_wait_us);
// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release_shared(badge_err_t *ec, mutex_t *mutex);
// Try to acquire a share in `mutex` within `max_wait_us` microseconds.
// If `max_wait_us` is too long or negative, do not use the timeout.
// Returns true if the share was successfully acquired.
bool mutex_acquire_shared_from_isr(badge_err_t *ec, mutex_t *mutex, timestamp_us_t max_wait_us);
// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release_shared_from_isr(badge_err_t *ec, mutex_t *mutex);
