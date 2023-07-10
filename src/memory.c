
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



// GCC will emit calls to memset and memcpy even if -ffreestanding -fno-builtin-memset and so on when LTO is enabled
// see https://github.com/riscv-collab/riscv-gnu-toolchain/issues/758

void *memset(void *dest, int ch, size_t count);
void *memset(void *dest, int ch, size_t count) {
    // mem_set(dest, ch, count);
    return dest;
}

void *memcpy(void *restrict dest, void const *restrict src, size_t count);
void *memcpy(void *restrict dest, void const *restrict src, size_t count) {
    // mem_copy(dest, src, count);
    return dest;
}