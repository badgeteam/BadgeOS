
// SPDX-License-Identifier: MIT

#include "assertions.h"

#include "cpu/panic.h"
#include "log.h"

void kernel_assertion_failure(char const *assertion_msg) {
    // Report error and cause debug break.
    logk(LOG_FATAL, assertion_msg);
    __builtin_unreachable();
}