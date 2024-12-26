
// SPDX-License-Identifier: MIT
// Port of hal/assert.h

#pragma once

#include "assertions.h"
#include "panic.h"

#define HAL_ASSERT(__e) assert_dev_drop(__e)
extern void abort();
