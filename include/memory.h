
// SPDX-License-Identifier: MIT

#pragma once

#include <stddef.h>
#include <stdint.h>

// Copies `length` bytes from `src` to `dst`
void mem_copy(void *dst, void const *src, size_t length);

// Sets `length` bytes in `dst` to `fill_byte`
void mem_set(void *dst, uint8_t fill_byte, size_t length);
