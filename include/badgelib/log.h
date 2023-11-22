
// SPDX-License-Identifier: MIT

#pragma once

#include <stddef.h>

typedef enum {
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
} log_level_t;

// Print an unformatted message.
void logk(log_level_t level, char const *msg);
// Print a formatted message according to format_str.
void logkf(log_level_t level, char const *msg, ...);
// Print a hexdump (usually for debug purposes).
void logk_hexdump(log_level_t level, char const *msg, void const *data, size_t size);
// Print a hexdump, override the address shown (usually for debug purposes).
void logk_hexdump_vaddr(log_level_t level, char const *msg, void const *data, size_t size, size_t vaddr);
