
// SPDX-License-Identifier: MIT

#include "mutex.h"

#include "assertions.h"
#include "scheduler.h"
#include "time.h"

// Magic value for exclusive locking.
#define EXCLUSIVE_MAGIC ((int)__INT_MAX__ / 4)



// Atomically await the expected value and swap in the new value.
static bool
    await_swap_atomic_int(atomic_int *var, timestamp_us_t timeout, int expected, int new_value, memory_order order) {
    do {
        int old_value = expected;
        if (atomic_compare_exchange_weak_explicit(var, &old_value, new_value, order, memory_order_relaxed)) {
            return true;
        } else {
            sched_yield();
        }
    } while (time_us() < timeout);
    return false;
}

// Atomically check the value does not exceed a threshold and add 1.
static bool thresh_add_atomic_int(atomic_int *var, timestamp_us_t timeout, int threshold, memory_order order) {
    do {
        int old_value = atomic_load(var);
        int new_value = old_value + 1;
        if (old_value >= threshold || new_value >= threshold) {
            return false;
        } else if (atomic_compare_exchange_weak_explicit(var, &old_value, new_value, order, memory_order_relaxed)) {
            return true;
        } else {
            sched_yield();
        }
    } while (time_us() < timeout);
    return false;
}

// Atomically check the value doesn't equal either illegal values and subtract 1.
static bool unequal_sub_atomic_int(atomic_int *var, int unequal0, int unequal1, memory_order order) {
    while (1) {
        int old_value = atomic_load(var);
        int new_value = old_value - 1;
        if (old_value == unequal0 || old_value == unequal1) {
            return false;
        } else if (atomic_compare_exchange_weak_explicit(var, &old_value, new_value, order, memory_order_relaxed)) {
            return true;
        } else {
            sched_yield();
        }
    }
}



// Initialise a mutex for unshared use.
void mutex_init(badge_err_t *ec, mutex_t *mutex) {
    if (atomic_load_explicit(&mutex->magic, memory_order_acquire) == MUTEX_MAGIC) {
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_ILLEGAL);
        return;
    }
    mutex->magic     = MUTEX_MAGIC;
    mutex->is_shared = false;
    atomic_store(&mutex->shares, 0);
    atomic_thread_fence(memory_order_release);
    badge_err_set_ok(ec);
}

// Initialise a mutex for shared use.
void mutex_init_shared(badge_err_t *ec, mutex_t *mutex) {
    if (atomic_load_explicit(&mutex->magic, memory_order_acquire) == MUTEX_MAGIC) {
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_ILLEGAL);
        return;
    }
    mutex->magic     = MUTEX_MAGIC;
    mutex->is_shared = true;
    atomic_store(&mutex->shares, 0);
    atomic_thread_fence(memory_order_release);
    badge_err_set_ok(ec);
}

// Clean up the mutex.
void mutex_destroy(badge_err_t *ec, mutex_t *mutex) {
    if (atomic_load_explicit(&mutex->magic, memory_order_acquire) != MUTEX_MAGIC) {
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_ILLEGAL);
        return;
    }
    mutex->magic = 0;
    atomic_thread_fence(memory_order_release);
}

// Try to acquire `mutex` within `timeout` microseconds.
// Returns true if the mutex was successully acquired.
bool mutex_acquire(badge_err_t *ec, mutex_t *mutex, timestamp_us_t timeout) {
    if (atomic_load_explicit(&mutex->magic, memory_order_acquire) != MUTEX_MAGIC) {
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_ILLEGAL);
        return false;
    }
    // Compute timeout.
    timestamp_us_t now = time_us();
    if (timeout < 0 || timeout - TIMESTAMP_US_MAX + now >= 0) {
        timeout = TIMESTAMP_US_MAX;
    } else {
        timeout += now;
    }
    // Await the shared portion to reach 0 and then lock.
    if (await_swap_atomic_int(&mutex->shares, timeout, 0, EXCLUSIVE_MAGIC, memory_order_acquire)) {
        // If that succeeds, the mutex was acquired.
        badge_err_set_ok(ec);
        return true;
    } else {
        // Acquire failed.
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_TIMEOUT);
        return false;
    }
}

// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release(badge_err_t *ec, mutex_t *mutex) {
    if (atomic_load_explicit(&mutex->magic, memory_order_acquire) != MUTEX_MAGIC) {
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_ILLEGAL);
        return false;
    }
    assert_dev_drop(atomic_load(&mutex->shares) >= EXCLUSIVE_MAGIC);
    if (await_swap_atomic_int(&mutex->shares, TIMESTAMP_US_MAX, EXCLUSIVE_MAGIC, 0, memory_order_release)) {
        // Successful release.
        badge_err_set_ok(ec);
        return true;
    } else {
        // Mutex was not taken exclusively.
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_ILLEGAL);
        return false;
    }
}

// Try to acquire a share in `mutex` within `timeout` microseconds.
// Returns true if the share was successfully acquired.
bool mutex_acquire_shared(badge_err_t *ec, mutex_t *mutex, timestamp_us_t timeout) {
    if (atomic_load_explicit(&mutex->magic, memory_order_acquire) != MUTEX_MAGIC) {
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_ILLEGAL);
        return false;
    }
    if (!mutex->is_shared) {
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_ILLEGAL);
        return false;
    }
    // Compute timeout.
    timestamp_us_t now = time_us();
    if (timeout < 0 || timeout - TIMESTAMP_US_MAX + now >= 0) {
        timeout = TIMESTAMP_US_MAX;
    } else {
        timeout += now;
    }
    // Take a share.
    if (thresh_add_atomic_int(&mutex->shares, timeout, EXCLUSIVE_MAGIC, memory_order_acquire)) {
        // If that succeeds, the mutex was successfully acquired.
        badge_err_set_ok(ec);
        return true;
    } else {
        // If that fails, abort trying to lock.
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_TIMEOUT);
        return false;
    }
}

// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release_shared(badge_err_t *ec, mutex_t *mutex) {
    if (atomic_load_explicit(&mutex->magic, memory_order_acquire) != MUTEX_MAGIC) {
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_ILLEGAL);
        return false;
    }
    assert_dev_drop(atomic_load(&mutex->shares) < EXCLUSIVE_MAGIC);
    if (!unequal_sub_atomic_int(&mutex->shares, 0, EXCLUSIVE_MAGIC, memory_order_release)) {
        // Prevent the counter from underflowing.
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_ILLEGAL);
        return false;
    } else {
        // Successful release.
        badge_err_set_ok(ec);
        return true;
    }
}
