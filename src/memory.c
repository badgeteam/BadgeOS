
// SPDX-License-Identifier: MIT

#include "memory.h"

void mem_copy(void *const dst, void const *const src, size_t const length) {
    char       *destination = dst;
    char const *source      = src;
    for (size_t i = 0; i < length; i++) {
        destination[i] = source[i];
    }
}

void mem_set(void *const dst, uint8_t const fill_byte, size_t const length) {
    char *destination = dst;
    for (size_t i = 0; i < length; i++) {
        destination[i] = fill_byte;
    }
}
