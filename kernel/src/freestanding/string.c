
// SPDX-License-Identifier: MIT

#include "badge_strings.h"



// Compute the length of a C-string.
size_t strlen(char const *string) {
    char const *ptr = string;
    while (*ptr) ptr++;
    return ptr - string;
}

// Compute the length of a C-string at most `max_length` characters long.
size_t strnlen(char const *string, size_t max_len) {
    for (size_t i = 0; i < max_len; i++) {
        if (!string[i])
            return i;
    }
    return max_len;
}

// Implementation of the mem_copy loop with variable access size.
#define MEM_COPY_IMPL(type, alignment, dest, src, size)                                                                \
    {                                                                                                                  \
        type       *dest_ptr = (dest); /* NOLINT */                                                                    \
        type const *src_ptr  = (src);  /* NOLINT */                                                                    \
        size_t      _size    = (size) / (alignment);                                                                   \
        if ((dest) < (src)) {                                                                                          \
            /* Forward iteration. */                                                                                   \
            for (size_t i = 0; i < _size; i++) {                                                                       \
                dest_ptr[i] = src_ptr[i];                                                                              \
            }                                                                                                          \
        } else if ((src) < (dest)) {                                                                                   \
            /* Reverse iteration. */                                                                                   \
            for (size_t i = _size; i-- > 0;) {                                                                         \
                dest_ptr[i] = src_ptr[i];                                                                              \
            }                                                                                                          \
        }                                                                                                              \
    }

// Copy the contents of memory area `src` to memory area `dest`.
// Correct copying is gauranteed even if `src` and `dest` are overlapping regions.
void *memmove(void *dest, void const *src, size_t size) {
    size_t align_detector = (size_t)dest | (size_t)src | (size_t)size;

    // Optimise for alignment.
    if (align_detector & 1) {
        MEM_COPY_IMPL(uint8_t, 1, dest, src, size)
    } else if (align_detector & 2) {
        MEM_COPY_IMPL(uint16_t, 2, dest, src, size)
    } else if (align_detector & 4) {
        MEM_COPY_IMPL(uint32_t, 4, dest, src, size)
    } else {
        MEM_COPY_IMPL(uint64_t, 8, dest, src, size)
    }

    return dest;
}

void *memcpy(void *dest, void const *src, size_t size) {
    return memmove(dest, src, size);
}


// Implementation of the mem_set loop with variable access size.
#define MEM_SET_IMPL(type, alignment, dest, value, size)                                                               \
    {                                                                                                                  \
        type  *dest_ptr = (dest); /* NOLINT */                                                                         \
        size_t _size    = (size) / (alignment);                                                                        \
        for (size_t i = 0; i < _size; i++) {                                                                           \
            dest_ptr[i] = (value);                                                                                     \
        }                                                                                                              \
    }

// Set the contents of memory area `dest` to the constant byte `value`.
void *memset(void *dest, uint8_t value, size_t size) {
    size_t align_detector = (size_t)dest | (size_t)size;

    // Optimise for alignment.
    if (align_detector & 1) {
        MEM_SET_IMPL(uint8_t, 1, dest, value, size)
    } else if (align_detector & 2) {
        MEM_SET_IMPL(uint16_t, 2, dest, value, size)
    } else if (align_detector & 4) {
        MEM_SET_IMPL(uint32_t, 4, dest, value, size)
    } else {
        MEM_SET_IMPL(uint64_t, 8, dest, value, size)
    }

    return dest;
}
