
// SPDX-License-Identifier: MIT

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Assertion that will only assert in debug builds, will be kept in release builds
#define assert_dev_keep(_Cond) (void)(_Cond)

// Assertion that will only assert in debug builds, will be removed in release builds
#define assert_dev_drop(_Cond) (void)0

// Assertion that will be always active
#define assert_prod(_Cond) (void)(_Cond)
