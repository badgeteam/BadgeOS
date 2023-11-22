
// SPDX-License-Identifier: MIT

// Replacement for stdlib's <stdlib.h>

#pragma once

#include <malloc.h>

#define abs(a)   __builtin_abs(a)
#define labs(a)  __builtin_labs(a)
#define llabs(a) __builtin_llabs(a)
