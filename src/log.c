
// SPDX-License-Identifier: MIT

#include "log.h"
#include "badge_format_str.h"
#include "rawprint.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define isvalidlevel(level) ((level) >= 0 && (level) < 5)



static char const *const prefix[] = {
    "FATAL ",
    "ERROR ",
    "WARN  ",
    "INFO  ",
    "DEBUG ",
};

static char const *const colcode[] = {
    "\033[31m",
    "\033[31m",
    "\033[33m",
    "\033[32m",
    "\033[34m",
};

static char const *const term = "\033[0m\r\n";



// Print an unformatted message.
void logk(log_level_t level, char const *msg) {
    if (isvalidlevel(level)) rawprint(colcode[level]);
    rawprintuptime();
    rawputc(' ');
    if (isvalidlevel(level)) rawprint(prefix[level]);
    else rawprint("      ");
    rawprint(msg);
    rawprint(term);
}



static bool putccb(char const *msg, size_t len, void *cookie) {
    while (len--) rawputc(*(msg++));
    return true;
}

// Print a formatted message.
void logkf(log_level_t level, char const *msg, ...) {
    if (isvalidlevel(level)) rawprint(colcode[level]);
    rawprintuptime();
    if (isvalidlevel(level)) rawprint(prefix[level]);
    else rawprint("      ");
    va_list vararg;
    va_start(vararg, msg);
    format_str(msg, putccb, NULL, vararg);
    va_end(vararg);
    rawprint(term);
}



// Print a hexdump (usually for debug purposes).
void logk_hexdump(log_level_t level, char const *msg, void const *data, size_t size) {
    logk_hexdump(level, msg, data, size, (size_t) data);
}

// Print a hexdump, override the address shown (usually for debug purposes).
void logk_hexdump_vaddr(log_level_t level, char const *msg, void const *data, size_t size, size_t vaddr) {
    logk(level, msg);
    if (isvalidlevel(level)) rawprint(colcode[level]);
    
    uint8_t const *ptr = data;
}
