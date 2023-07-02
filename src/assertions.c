#include "assertions.h"

#include "log.h"
#include "panic.h"

void kernel_assertion_failure(char const *assertion_msg) {
    // TODO: Panic here
    logk(LOG_FATAL, assertion_msg);

    panic_abort();
}