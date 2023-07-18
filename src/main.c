
// SPDX-License-Identifier: MIT

#include "assertions.h"
#include "attributes.h"
#include "gpio.h"
#include "log.h"
#include "port/interrupt.h"
#include "rawprint.h"
#include "scheduler.h"
#include "time.h"

#include <stdint.h>

// Temporary kernel context until threading is implemented.
static kernel_ctx_t kctx;

static void         led_blink_loop(void *);
static void         uart_print_loop(void *);

static uint8_t      led_blink_stack[256] ALIGNED_TO(STACK_ALIGNMENT);
static uint8_t      uart_print_stack[256] ALIGNED_TO(STACK_ALIGNMENT);

// This is the entrypoint after the stack has been set up and the init functions
// have been run. Main is not allowed to return, so declare it noreturn.
void main() NORETURN;
void main() {
    badge_err_t err;

    // Install interrupt and trap handlers.
    interrupt_init(&kctx);

    // Set up timers and watchdogs.
    // This function must run within the first ~1s of power-on time and should be
    // called as early as possible.
    time_init();

    // Test a log message.
    logk(LOG_FATAL, "The ultimage log message test");
    logk(LOG_ERROR, "The ultimage log message test");
    logk(LOG_WARN, "The ultimage log message test");
    logk(LOG_INFO, "The ultimage log message test");
    logk(LOG_DEBUG, "The ultimage log message test");

    // Test a GPIO.
    io_mode(NULL, 15, IO_MODE_OUTPUT);
    io_mode(NULL, 22, IO_MODE_INPUT);
    io_pull(NULL, 22, IO_PULL_UP);


    sched_init(&err);
    assert_always(badge_err_is_ok(&err));


    sched_thread_t *const led_thread = sched_create_kernel_thread(
        &err,
        led_blink_loop,
        NULL,
        led_blink_stack,
        sizeof led_blink_stack,
        SCHED_PRIO_NORMAL
    );
    assert_always(badge_err_is_ok(&err));
    assert_always(led_thread != NULL);

    sched_thread_t *const print_thread = sched_create_kernel_thread(
        &err,
        uart_print_loop,
        NULL,
        uart_print_stack,
        sizeof uart_print_stack,
        SCHED_PRIO_NORMAL
    );
    assert_always(badge_err_is_ok(&err));
    assert_always(print_thread != NULL);

    sched_set_name(&err, led_thread, "led");
    assert_always(badge_err_is_ok(&err));

    sched_set_name(&err, print_thread, "print");
    assert_always(badge_err_is_ok(&err));

    sched_resume_thread(&err, led_thread);
    assert_always(badge_err_is_ok(&err));

    sched_resume_thread(&err, print_thread);
    assert_always(badge_err_is_ok(&err));


    // Enter the scheduler
    sched_exec();

    __builtin_unreachable();
}

static void led_blink_loop(void *) {

    bool status = false;
    while (1) {

        io_write(NULL, 15, status);
        status            = !status;

        int64_t const now = time_us();
        while (time_us() < now + 500LL * TIME_US_PER_MS) {
            logk(LOG_DEBUG, "<yield process=led>");
            sched_yield();
            logk(LOG_DEBUG, "</yield process=led>");
        }
    }
}

static void uart_print_loop(void *) {

    while (1) {
        int64_t const now = time_us();


        while (time_us() < now + 700LL * TIME_US_PER_MS) {
            logk(LOG_DEBUG, "<yield process=uart>");
            sched_yield();
            logk(LOG_DEBUG, "</yield process=uart>");
        }
        logk(LOG_INFO, "timer");
    }
}