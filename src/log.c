
// SPDX-License-Identifier: MIT

#include "log.h"
#include "rawprint.h"

typedef __builtin_va_list va_list;
#define va_start(x) __buitlin_va_start(x)
#define va_end(x) __builtin_va_end(x)
#define va_arg(x,y) __builtin_va_arg(x,y)

#define isvalidlevel(level) ((level) >= 0 && (level) < 5)

static const char *prefix[] = {
	"FATAL ",
	"ERROR ",
	"WARN  ",
	"INFO  ",
	"DEBUG ",
};

static const char *colcode[] = {
	"\033[31m",
	"\033[31m",
	"\033[33m",
	"\033[32m",
	"\033[34m",
};

static const char *term = "\033[0m\r\n";

// Print an unformatted message.
void logk(log_level_t level, const char *msg) {
	if (isvalidlevel(level)) rawprint(colcode[level]);
	rawprintuptime();
	rawputc(' ');
	if (isvalidlevel(level)) rawprint(prefix[level]);
	else rawprint("      ");
	rawprint(msg);
	rawprint(term);
}

// Print a formatted message.
void logkf(log_level_t level, const char *msg, ...) {
	if (isvalidlevel(level)) rawprint(colcode[level]);
	rawprintuptime();
	if (isvalidlevel(level)) rawprint(prefix[level]);
	else rawprint("      ");
	rawprint(msg);
	rawprint(term);
}
