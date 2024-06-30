
// SPDX-License-Identifier: MIT

#pragma once

#include "attributes.h"
#include "log.h"
#include "meta.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if __STDC_VERSION__ < 201112L // C11
#error "Requires at least C11!"
#elif __STDC_VERSION__ < 202311L // C23
#define static_assert _Static_assert
#endif



// Assert based on a condition in both debug and release builds.
#define assert_always(condition)                                                                                       \
    do {                                                                                                               \
        if (__builtin_expect((condition) == 0, 0)) {                                                                   \
            logk(                                                                                                      \
                LOG_FATAL,                                                                                             \
                __FILE__ ":" convert_macro_to_string(__LINE__) ": " convert_macro_to_string(__func__                   \
                ) ": Assertion `" #condition "` failed."                                                               \
            );                                                                                                         \
            __builtin_trap();                                                                                          \
        }                                                                                                              \
    } while (false)



#ifdef NDEBUG
// Assert based on a condition in debug builds, run normally in release builds.
#define assert_dev_keep(condition) ((void)(condition))
// Assert based on a condition in debug builds.
#define assert_dev_drop(condition) (void)0
#else
// Assert based on a condition in debug builds, run normally in release builds.
#define assert_dev_keep(condition) assert_always(condition)
// Assert based on a condition in debug builds.
#define assert_dev_drop(condition) assert_always(condition)
#endif
