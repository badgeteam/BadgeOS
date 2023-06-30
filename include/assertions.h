
// SPDX-License-Identifier: MIT

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "attributes.h"
#include "meta.h"

#if __STDC_VERSION__ < 201112L // C11
#error "Requires at least C11!"
#elif __STDC_VERSION__ < 202311L // C23
#define static_assert _Static_assert
#endif

void kernel_assertion_failure(char const *assertion_msg) NORETURN;

// Assertion that will be always active
#define assert_always(condition)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if ((condition) == 0)                                                                                          \
        {                                                                                                              \
            kernel_assertion_failure(__FILE__ ":" convert_macro_to_string(__LINE__) ": " convert_macro_to_string(      \
                __func__) ": Assertion `" #condition "` failed.");                                                     \
        }                                                                                                              \
    } while (false)

#ifdef NDEBUG

// Assertion that will only assert in debug builds, will be kept in release builds
#define assert_dev_keep(condition) (void)(condition)

// Assertion that will only assert in debug builds, will be removed in release builds
#define assert_dev_drop(condition) (void)0

#else

// Assertion that will only assert in debug builds, will be kept in release builds
#define assert_dev_keep(condition) assert_always(condition)

// Assertion that will only assert in debug builds, will be removed in release builds
#define assert_dev_drop(condition) assert_always(condition)

#endif
