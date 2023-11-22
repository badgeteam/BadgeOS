
// SPDX-License-Identifier: MIT

// Replacement for stdlib's <assert.h>

#pragma once

#include "badgelib/assertions.h"

#define assert(x) assert_dev_drop(x)
