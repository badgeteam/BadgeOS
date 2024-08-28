
// SPDX-License-Identifier: MIT

#include "time.h"

#include "arrays.h"
#include "assertions.h"
#include "config.h"
#include "hwtimer.h"
#include "interrupt.h"
#include "mutex.h"
#include "scheduler/isr.h"
#include "smp.h"

#ifdef CONFIG_TARGET_esp32p4
// Timer used for task list items.
#define TT_TIMER 2
#else
// Timer used for task list items.
#define TT_TIMER 1
#endif



// Callback to the timer driver for when a timer alarm fires.
void timer_isr_timer_alarm() {
    int cpu = smp_cur_cpu();
    timer_alarm_disable(cpu);
    timer_int_clear(cpu);
    sched_request_switch_from_isr();
}



// Timer task list entry.
typedef struct {
    // Task ID.
    int64_t        taskno;
    // Task timestamp;
    timestamp_us_t time;
    // Task function.
    timer_fn_t     callback;
    // Task cookie.
    void          *cookie;
} timertask_t;

// Timer task mutex.
static mutex_t        tt_mtx      = MUTEX_T_INIT;
// Next timer task ID.
static int64_t        next_taskno = 1;
// Timer task list capacity.
static size_t         tt_list_cap;
// Timer task list length.
static size_t         tt_list_len;
// Timer task list.
static timertask_t   *tt_list;
// Current limit of task list timer.
static timestamp_us_t tt_limit = TIMESTAMP_US_MAX;

// Sort `timertask_t` by timestamp.
int tt_cmp(void const *a, void const *b) {
    timertask_t const *a_ptr = a;
    timertask_t const *b_ptr = b;
    if (a_ptr->time < b_ptr->time) {
        return -1;
    } else if (a_ptr->time > b_ptr->time) {
        return 1;
    } else {
        return 0;
    }
}

// Timer task ISR.
void tt_isr(int irq) {
    (void)irq;
    mutex_acquire_from_isr(NULL, &tt_mtx, TIMESTAMP_US_MAX);

    // Disable alarm.
    timer_alarm_disable(TT_TIMER);
    timer_int_clear(TT_TIMER);

    // Consume all tasks for this timestamp.
    timestamp_us_t now = time_us();
    size_t         i;
    for (i = 0; i < tt_list_len && tt_list[i].time <= now; i++) {
        tt_list[i].callback(tt_list[i].cookie);
    }
    array_lencap_remove_n(&tt_list, sizeof(timertask_t), &tt_list_len, &tt_list_cap, NULL, 0, i);

    if (tt_list_len) {
        // Set next timer.
        tt_limit = tt_list[0].time;
        timer_alarm_config(TT_TIMER, tt_limit, false);
    } else {
        tt_limit = TIMESTAMP_US_MAX;
    }

    mutex_release_from_isr(NULL, &tt_mtx);
}



// Initialise timer and watchdog subsystem.
void time_init() {
    timer_init();

    // Configure system timers.
    for (int i = 0; i <= TT_TIMER; i++) {
        timer_stop(i);
        timer_value_set(i, 0);
        timer_alarm_disable(i);
        timer_int_clear(i);
        timer_int_enable(i, true);
        timer_set_freq(i, 1000000);
    }

    // Assign timer IRQs.
    for (int i = 0; i < TT_TIMER; i++) {
        set_cpu_timer_irq(i, timer_get_irq(i));
    }
    int tt_irq = timer_get_irq(TT_TIMER);
    irq_ch_set_isr(tt_irq, tt_isr);
    irq_ch_enable(tt_irq);

    // Start timers at close to the same time.
    for (int i = 0; i <= TT_TIMER; i++) {
        timer_start(i);
    }
}

// Sets the alarm time when the next task switch should occur.
void time_set_next_task_switch(timestamp_us_t timestamp) {
    timer_alarm_config(smp_cur_cpu(), timestamp, false);
}

// Attach a task to a timer interrupt.
int64_t time_add_async_task(timestamp_us_t timestamp, timer_fn_t taskfn, void *cookie) {
    if (timestamp <= 0) {
        return 0;
    }
    bool ie = irq_disable();
    mutex_acquire_from_isr(NULL, &tt_mtx, TIMESTAMP_US_MAX);
    timertask_t task = {
        .callback = taskfn,
        .cookie   = cookie,
        .time     = timestamp,
        .taskno   = next_taskno,
    };
    int64_t taskno = 0;
    if (array_lencap_sorted_insert(&tt_list, sizeof(timertask_t), &tt_list_len, &tt_list_cap, &task, tt_cmp)) {
        taskno = next_taskno;
        next_taskno++;
    }
    if (tt_limit > timestamp) {
        tt_limit = timestamp;
        timer_alarm_config(TT_TIMER, tt_limit, false);
    }
    mutex_release_from_isr(NULL, &tt_mtx);
    irq_enable_if(ie);
    return taskno;
}

// Cancel a task created with `time_add_async_task`.
bool time_cancel_async_task(int64_t taskno) {
    bool ie      = irq_disable();
    bool success = false;
    mutex_acquire_from_isr(NULL, &tt_mtx, TIMESTAMP_US_MAX);
    for (size_t i = 0; i < tt_list_len; i++) {
        if (tt_list[i].taskno == taskno) {
            array_lencap_remove(&tt_list, sizeof(timertask_t), &tt_list_len, &tt_list_cap, NULL, i);
            success = true;
        }
    }
    mutex_release_from_isr(NULL, &tt_mtx);
    irq_enable_if(ie);
    return success;
}

// Get current time in microseconds.
timestamp_us_t time_us() {
    return timer_value_get(smp_cur_cpu());
}
