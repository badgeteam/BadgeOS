
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include "log.h"
#include "time.h"
#include "gpio.h"
#include "port/interrupt.h"



// Temporary kernel context until threading is implemented.
static kernel_ctx_t kctx;
// Temporary registers struct until threading is implemented.
static cpu_regs_t regs;

// This is the entrypoint after the stack has been set up and the init functions have been run.
// Main is not allowed to return, so declare it noreturn.
void main() __attribute__((noreturn));
void main() {
	// Install interrupt and trap handlers.
	interrupt_init(&kctx, &regs);
	
	// Set up timers and watchdogs.
	// This function must run within the first ~1s of power-on time and should be called as early as possible.
	time_init();
	
	// Test a log message.
	logk(LOG_FATAL, "The ultimage log message test");
	logk(LOG_ERROR, "The ultimage log message test");
	logk(LOG_WARN,  "The ultimage log message test");
	logk(LOG_INFO,  "The ultimage log message test");
	logk(LOG_DEBUG, "The ultimage log message test");
	
	// Test a GPIO.
	io_mode(NULL, 15, IO_MODE_OUTPUT);
	io_mode(NULL, 22, IO_MODE_INPUT);
	io_pull(NULL, 22, IO_PULL_UP);
	while (1) {
		int64_t now = time_us();
		io_write(NULL, 15, ((now / 1000000) & 1) ^ io_read(NULL, 22));
	}
}
