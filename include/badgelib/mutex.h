
// SPDX-License-Identifier: MIT

#pragma once

#include "badge_err.h"
#include "time.h"

#include <stdatomic.h>

// Magic value for the magic field.
#define MUTEX_MAGIC (int)0xcafebabe

typedef struct {
    // Magic value.
    atomic_int magic;
    // Mutex allows sharing.
    bool       is_shared;
    // Share count and/or is locked.
    atomic_int shares;
} mutex_t;

#define MUTEX_T_INIT        ((mutex_t){MUTEX_MAGIC, 0, 0})
#define MUTEX_T_INIT_SHARED ((mutex_t){MUTEX_MAGIC, 1, 0})

// Initialise a mutex for unshared use.
void mutex_init(badge_err_t *ec, mutex_t *mutex);
// Initialise a mutex for shared use.
void mutex_init_shared(badge_err_t *ec, mutex_t *mutex);
// Clean up the mutex.
void mutex_destroy(badge_err_t *ec, mutex_t *mutex);
// Try to acquire `mutex` within `max_wait_us` microseconds.
// If `max_wait_us` is too long or negative, do not use the timeout.
// Returns true if the mutex was successully acquired.
bool mutex_acquire(badge_err_t *ec, mutex_t *mutex, timestamp_us_t max_wait_us);
// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release(badge_err_t *ec, mutex_t *mutex);
// Try to acquire a share in `mutex` within `max_wait_us` microseconds.
// If `max_wait_us` is too long or negative, do not use the timeout.
// Returns true if the share was successfully acquired.
bool mutex_acquire_shared(badge_err_t *ec, mutex_t *mutex, timestamp_us_t max_wait_us);
// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release_shared(badge_err_t *ec, mutex_t *mutex);
