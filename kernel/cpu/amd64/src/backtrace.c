
// SPDX-License-Identifier: MIT

#include "backtrace.h"

#include "attributes.h"
#include "isr_ctx.h"
#include "rawprint.h"

#include <stddef.h>

#ifndef BACKTRACE_DEPTH
#define BACKTRACE_DEPTH 32
#endif



// Given stack frame pointer, perform backtrace.
void backtrace_from_ptr(void *frame_pointer) {
    rawprint("**** BEGIN BACKRTACE ****\n");
    // TODO.
    rawprint("**** END BACKRTACE ****\n");
}

// Perform backtrace as called.
void backtrace() NAKED;
void backtrace() {
    // TODO.
    asm volatile("ret");
}
