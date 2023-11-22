
// SPDX-License-Identifier: MIT

#include <badge_strings.h>



// Compute the length of a C-string.
size_t cstr_length(char const *string) {
    char const *ptr = string;
    while (*ptr) ptr++;
    return ptr - string;
}

// Compute the length of a C-string at most `max_length` characters long.
size_t cstr_length_upto(char const *string, size_t max_len) {
    for (size_t i = 0; i < max_len; i++) {
        if (!string[i])
            return i;
    }
    return max_len;
}

// Find the first occurrance `value` in C-string `string`.
// Returns -1 when not found.
ptrdiff_t cstr_index(char const *string, char value) {
    char const *ptr = string;

    // Find first index.
    while (*ptr) {
        if (*ptr == value)
            return ptr - string;
        ptr++;
    }

    return -1;
}

// Find the last occurrance `value` in C-string `string`.
// Returns -1 when not found.
ptrdiff_t cstr_last_index(char const *string, char value) {
    return cstr_last_index_upto(string, value, SIZE_MAX);
}

// Find the first occurrance `value` in C-string `string` from and including `first_index`.
// Returns -1 when not found.
ptrdiff_t cstr_index_from(char const *string, char value, size_t first_index) {
    char const *ptr = string + first_index;

    // Make sure string is long enough.
    for (size_t i = 0; i < first_index; i++) {
        if (!string[i])
            return -1;
    }

    // Find first index.
    while (*ptr) {
        if (*ptr == value)
            return ptr - string;
        ptr++;
    }

    return -1;
}

// Find the last occurrance `value` in C-string `string` up to and excluding `last_index`.
// Returns -1 when not found.
ptrdiff_t cstr_last_index_upto(char const *string, char value, size_t last_index) {
    ptrdiff_t found = -1;
    for (size_t i = 0; string[i] && i < last_index; i++) {
        if (string[i] == value)
            found = (ptrdiff_t)i;
    }
    return found;
}



// Test the equality of two C-strings.
bool cstr_equals(char const *a, char const *b) {
    while (1) {
        if (*a != *b)
            return false;
        if (!*a)
            return true;
        a++, b++;
    }
}

// Test the of the first `length` characters equality of two C-strings.
bool cstr_prefix_equals(char const *a, char const *b, size_t length) {
    while (length--) {
        if (*a != *b)
            return false;
        if (!*a)
            return true;
        a++, b++;
    }
    return true;
}

// Test the equality of two C-strings, case-insensitive.
bool cstr_equals_case(char const *a, char const *b) {
    while (1) {
        if (ascii_char_to_lower(*a) != ascii_char_to_lower(*b))
            return false;
        if (!*a)
            return true;
        a++, b++;
    }
}

// Test the of the first `length` characters equality of two C-strings, case-insensitive.
bool cstr_prefix_equals_case(char const *a, char const *b, size_t length) {
    while (length--) {
        if (ascii_char_to_lower(*a) != ascii_char_to_lower(*b))
            return false;
        if (!*a)
            return true;
        a++, b++;
    }
    return true;
}



// Concatenate a NULL-terminated C-string from `src` onto C-string buffer `dest`.
// This may truncate characters, but not the NULL terminator, if `dest` does not fit `src` entirely.
size_t cstr_concat(char *dest, size_t size, char const *src) {
    size_t dest_len = cstr_length(dest);
    if (dest_len < size - 1) {
        return cstr_copy(dest + dest_len, size - dest_len, src) + dest_len;
    }
    return dest_len;
}

// Concatenate a NULL-terminated C-string from `src` onto C-string buffer `dest`.
// This may truncate characters, but not the NULL terminator, if `dest` does not fit `src` entirely.
size_t cstr_concat_packed(char *dest, size_t size, char const *src) {
    size_t dest_len = cstr_length_upto(dest, size);
    if (dest_len < size) {
        return cstr_copy_packed(dest + dest_len, size - dest_len, src) + dest_len;
    }
    return dest_len;
}

// Copy a NULL-terminated C-string from `src` into buffer `dest`.
// This may truncate characters, but not the NULL terminator, if `dest` does not fit `src` entirely.
size_t cstr_copy(char *dest, size_t size, char const *src) {
    char const *const orig = dest;
    while (size > 1) {
        if (!*src)
            break;
        *dest = *src;
        dest++, src++;
        size--;
    }
    if (size) {
        *dest = 0;
    }
    return dest - orig;
}

// Copy at most `length` bytes C-string `src` into buffer `dest`.
// WARNING: This may leave strings without NULL terminators if `dest` does not fit `src` entirely.
size_t cstr_copy_packed(char *dest, size_t size, char const *src) {
    char const *const orig = dest;
    while (size--) {
        *dest = *src;
        if (!*src)
            return dest - orig;
        dest++, src++;
    }
    return dest - orig;
}



// Find the first occurrance of byte `value` in memory `memory`.
// Returns -1 when not found.
ptrdiff_t mem_index(void const *memory, size_t size, uint8_t value) {
    uint8_t const *ptr = memory;
    for (size_t i = 0; i < size; i++) {
        if (ptr[i] == value)
            return (ptrdiff_t)i;
    }
    return -1;
}

// Find the first occurrance of byte `value` in memory `memory`.
// Returns -1 when not found.
ptrdiff_t mem_last_index(void const *memory, size_t size, uint8_t value) {
    uint8_t const *ptr = memory;
    for (size_t i = size; i-- > 0;) {
        if (ptr[i] == value)
            return (ptrdiff_t)i;
    }
    return -1;
}

// Implementation of the mem_equals loop with variable read size.
#define MEM_EQUALS_IMPL(type, alignment, a, b, size)                                                                   \
    {                                                                                                                  \
        type const *a_ptr = (a); /* NOLINT*/                                                                           \
        type const *b_ptr = (b); /* NOLINT*/                                                                           \
        size_t      _size = (size) / (alignment);                                                                      \
        for (size_t i = 0; i < _size; i++) {                                                                           \
            if (a_ptr[i] != b_ptr[i])                                                                                  \
                return false;                                                                                          \
        }                                                                                                              \
    }

// Test the equality of two memory areas.
bool mem_equals(void const *a, void const *b, size_t size) {
    size_t align_detector = (size_t)a | (size_t)b | (size_t)size;

    // Optimise for alignment.
    if (align_detector & 1) {
        MEM_EQUALS_IMPL(uint8_t, 1, a, b, size)
    } else if (align_detector & 2) {
        MEM_EQUALS_IMPL(uint16_t, 2, a, b, size)
    } else if (align_detector & 4) {
        MEM_EQUALS_IMPL(uint32_t, 4, a, b, size)
    } else {
        MEM_EQUALS_IMPL(uint64_t, 8, a, b, size)
    }

    return true;
}



// Implementation of the mem_copy loop with variable access size.
#define MEM_COPY_IMPL(type, alignment, dest, src, size)                                                                \
    {                                                                                                                  \
        type       *dest_ptr = (dest); /* NOLINT*/                                                                     \
        type const *src_ptr  = (src);  /* NOLINT*/                                                                     \
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
void mem_copy(void *dest, void const *src, size_t size) {
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
}



// Implementation of the mem_swap loop with variable access size.
#define MEM_SWAP_IMPL(type, alignment, a, b, size)                                                                     \
    {                                                                                                                  \
        type  *a_ptr = (a); /* NOLINT*/                                                                                \
        type  *b_ptr = (b); /* NOLINT*/                                                                                \
        size_t _size = (size) / (alignment);                                                                           \
        type   tmp;                                                                                                    \
        for (size_t i = 0; i < _size; i++) {                                                                           \
            tmp      = a_ptr[i];                                                                                       \
            a_ptr[i] = b_ptr[i];                                                                                       \
            b_ptr[i] = tmp;                                                                                            \
        }                                                                                                              \
    }

// Swap the contents of memory areas `a` and `b`.
// For correct copying, `a` and `b` must not overlap.
void mem_swap(void *a, void *b, size_t size) {
    size_t align_detector = (size_t)a | (size_t)b | (size_t)size;

    // Optimise for alignment.
    if (align_detector & 1) {
        MEM_SWAP_IMPL(uint8_t, 1, a, b, size)
    } else if (align_detector & 2) {
        MEM_SWAP_IMPL(uint16_t, 2, a, b, size)
    } else if (align_detector & 4) {
        MEM_SWAP_IMPL(uint32_t, 4, a, b, size)
    } else {
        MEM_SWAP_IMPL(uint64_t, 8, a, b, size)
    }
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
void mem_set(void *dest, uint8_t value, size_t size) {
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
}



// Function call emitted by the compiler.
void *memset(void *dst, int byte, size_t len) {
    mem_set(dst, (uint8_t)byte, len);
    return dst;
}

// Function call emitted by the compiler.
void *memcpy(void *dst, void const *src, size_t len) {
    mem_copy(dst, src, len);
    return dst;
}

// Function call emitted by the compiler.
void *memmove(void *dst, void const *src, size_t len) {
    mem_copy(dst, src, len);
    return dst;
}

// Function call emitted by the compiler.
int memcmp(void const *a, void const *b, size_t len) {
    // This is not strictly correct according to the `memcmp` spec,
    // but it will work for equality tests.
    return !mem_equals(a, b, len);
}
