
// SPDX-License-Identifier: MIT

#include "mutex.h"

#include "assertions.h"
#include "cpu/isr.h"
#include "interrupt.h"
#include "log.h"
#include "scheduler/isr.h"
#include "scheduler/scheduler.h"
#include "scheduler/types.h"
#include "smp.h"
#include "time.h"

// Magic value for exclusive locking.
#define EXCLUSIVE_MAGIC ((int)__INT_MAX__ / 4)



// Recommended way to create a mutex at run-time.
void mutex_init(badge_err_t *ec, mutex_t *mutex, bool shared, bool allow_isr) {
    *mutex = ((mutex_t){shared, allow_isr, ATOMIC_FLAG_INIT, 0, {0}});
    atomic_thread_fence(memory_order_release);
}

// Clean up the mutex.
void mutex_destroy(badge_err_t *ec, mutex_t *mutex) {
    // The mutex must always be completely unlocked to guarantee no threads are waiting on it.
    assert_dev_drop(mutex->shares == 0);
    assert_dev_drop(!atomic_flag_test_and_set(&mutex->wait_spinlock));
    irq_disable();
    int cur_cpu = smp_cur_cpu();
    while (mutex->waiting_list.len) {
        thread_handoff(dlist_pop_front(&mutex->waiting_list), cur_cpu, true, 0);
    }
    irq_enable();
}



// Mutex resume by timer.
static void mutex_resume_timer(void *cookie) {
}

// Mutex awaiting implementation.
static void mutex_wait(mutex_t *mutex, timestamp_us_t timeout) {
    // Disable IRQs because of multiple IRQ spinlocks in use here.
    irq_disable();
    // Pause the execution of this thread.
    sched_thread_t *self = thread_dequeue_self();

    if (timeout < TIMESTAMP_US_MAX) {
        // Set timeout interrupt for mutex.
        self->mutex_timer_id = time_add_async_task(timeout, mutex_resume_timer, self);
    } else {
        // No timeout; no timer interrupt is added.
        self->mutex_timer_id = 0;
    }

    // Add thread to mutex waiting list.
    while (atomic_flag_test_and_set_explicit(&mutex->wait_spinlock, memory_order_acquire));
    atomic_flag_clear_explicit(&mutex->wait_spinlock, memory_order_release);

    // Switch to some other still runnable thread.
    thread_yield();
}

// Atomically await the expected value and swap in the new value.
static inline bool await_swap_atomic_int(
    mutex_t *mutex, timestamp_us_t timeout, int expected, int new_value, memory_order order, bool from_isr
) {
    do {
        int old_value = expected;
        if (atomic_compare_exchange_weak_explicit(&mutex->shares, &old_value, new_value, order, memory_order_relaxed)) {
            return true;
        } else if (from_isr) {
            isr_pause();
        } else {
            mutex_wait(mutex, timeout);
        }
    } while (time_us() < timeout);
    return false;
}

// Atomically check the value does not exceed a threshold and add 1.
static inline bool
    thresh_add_atomic_int(mutex_t *mutex, timestamp_us_t timeout, int threshold, memory_order order, bool from_isr) {
    int old_value = atomic_load(&mutex->shares);
    do {
        int new_value = old_value + 1;
        if (!(old_value >= threshold || new_value >= threshold) &&
            atomic_compare_exchange_weak_explicit(&mutex->shares, &old_value, new_value, order, memory_order_relaxed)) {
            return true;
        } else if (from_isr) {
            isr_pause();
        } else {
            mutex_wait(mutex, timeout);
        }
    } while (time_us() < timeout);
    return false;
}

// Atomically check the value doesn't equal either illegal values and subtract 1.
static inline bool
    unequal_sub_atomic_int(mutex_t *mutex, int unequal0, int unequal1, memory_order order, bool from_isr) {
    int old_value = atomic_load(&mutex->shares);
    while (1) {
        int new_value = old_value - 1;
        if (!(old_value == unequal0 || old_value == unequal1) &&
            atomic_compare_exchange_weak_explicit(&mutex->shares, &old_value, new_value, order, memory_order_relaxed)) {
            return true;
        } else if (from_isr) {
            isr_pause();
        } else {
            mutex_wait(mutex, timeout);
        }
    }
}



// Try to acquire `mutex` within `timeout` microseconds.
// Returns true if the mutex was successully acquired.
static bool mutex_acquire_impl(badge_err_t *ec, mutex_t *mutex, timestamp_us_t timeout, bool from_isr) {
    assert_dev_drop(!from_isr || mutex->allow_isr);
    // Compute timeout.
    timestamp_us_t now = time_us();
    if (timeout < 0 || timeout - TIMESTAMP_US_MAX + now >= 0) {
        timeout = TIMESTAMP_US_MAX;
    } else {
        timeout += now;
    }
    // Await the shared portion to reach 0 and then lock.
    if (await_swap_atomic_int(&mutex->shares, timeout, 0, EXCLUSIVE_MAGIC, memory_order_acquire, from_isr)) {
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
static bool mutex_release_impl(badge_err_t *ec, mutex_t *mutex, bool from_isr) {
    assert_dev_drop(!from_isr || mutex->allow_isr);
    assert_dev_drop(atomic_load(&mutex->shares) >= EXCLUSIVE_MAGIC);
    if (await_swap_atomic_int(&mutex->shares, TIMESTAMP_US_MAX, EXCLUSIVE_MAGIC, 0, memory_order_release, from_isr)) {
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
static bool mutex_acquire_shared_impl(badge_err_t *ec, mutex_t *mutex, timestamp_us_t timeout, bool from_isr) {
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
    if (thresh_add_atomic_int(&mutex->shares, timeout, EXCLUSIVE_MAGIC, memory_order_acquire, from_isr)) {
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
static bool mutex_release_shared_impl(badge_err_t *ec, mutex_t *mutex, bool from_isr) {
    assert_dev_drop(atomic_load(&mutex->shares) < EXCLUSIVE_MAGIC);
    if (!unequal_sub_atomic_int(&mutex->shares, 0, EXCLUSIVE_MAGIC, memory_order_release, from_isr)) {
        // Prevent the counter from underflowing.
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_ILLEGAL);
        return false;
    } else {
        // Successful release.
        badge_err_set_ok(ec);
        return true;
    }
}


// Try to acquire `mutex` within `max_wait_us` microseconds.
// If `max_wait_us` is too long or negative, do not use the timeout.
// Returns true if the mutex was successully acquired.
bool mutex_acquire(badge_err_t *ec, mutex_t *mutex, timestamp_us_t max_wait_us) {
    return mutex_acquire_impl(ec, mutex, max_wait_us, false);
}
// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release(badge_err_t *ec, mutex_t *mutex) {
    return mutex_release_impl(ec, mutex, false);
}
// Try to acquire `mutex` within `max_wait_us` microseconds.
// If `max_wait_us` is too long or negative, do not use the timeout.
// Returns true if the mutex was successully acquired.
bool mutex_acquire_from_isr(badge_err_t *ec, mutex_t *mutex, timestamp_us_t max_wait_us) {
    return mutex_acquire_impl(ec, mutex, max_wait_us, true);
}
// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release_from_isr(badge_err_t *ec, mutex_t *mutex) {
    return mutex_release_impl(ec, mutex, true);
}

// Try to acquire a share in `mutex` within `max_wait_us` microseconds.
// If `max_wait_us` is too long or negative, do not use the timeout.
// Returns true if the share was successfully acquired.
bool mutex_acquire_shared(badge_err_t *ec, mutex_t *mutex, timestamp_us_t max_wait_us) {
    return mutex_acquire_shared_impl(ec, mutex, max_wait_us, false);
}
// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release_shared(badge_err_t *ec, mutex_t *mutex) {
    return mutex_release_shared_impl(ec, mutex, false);
}
// Try to acquire a share in `mutex` within `max_wait_us` microseconds.
// If `max_wait_us` is too long or negative, do not use the timeout.
// Returns true if the share was successfully acquired.
bool mutex_acquire_shared_from_isr(badge_err_t *ec, mutex_t *mutex, timestamp_us_t max_wait_us) {
    return mutex_acquire_shared_impl(ec, mutex, max_wait_us, true);
}
// Release `mutex`, if it was initially acquired by this thread.
// Returns true if the mutex was successfully released.
bool mutex_release_shared_from_isr(badge_err_t *ec, mutex_t *mutex) {
    return mutex_release_shared_impl(ec, mutex, true);
}
