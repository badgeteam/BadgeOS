
// SPDX-License-Identifier: MIT

// Replacement for stdlib's <stddef.h>

#pragma once

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __SIZE_TYPE__    size_t;
typedef __WCHAR_TYPE__   wchar_t;

#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif

#define offsetof(a, b) __builtin_offsetof(a, b)
#define alignof(a)     __builtin_alignof(a)
