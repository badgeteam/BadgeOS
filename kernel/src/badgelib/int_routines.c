
// SPDX-License-Identifier: MIT

// This file contains definitions for all GCC routines for integer arithmetic with the exception of int bit shifts.

// NOLINTBEGIN

#pragma GCC optimize("O2")

#include <stdbool.h>
#include <stdint.h>


typedef int          si_t __attribute__((mode(SI)));
typedef unsigned int usi_t __attribute__((mode(SI)));
typedef int          di_t __attribute__((mode(DI)));
typedef unsigned int udi_t __attribute__((mode(DI)));
#if __SIZE_MAX__ > 0xffffffff
#define do_ti_math
typedef int          ti_t __attribute__((mode(TI)));
typedef unsigned int uti_t __attribute__((mode(TI)));
#endif


// Implement division and modulo for an unsigned type.
// Does not do any edge case checks; divisor must never be 0.
#define DIVMOD_IMPL(type, name)                                                                                        \
    typedef struct {                                                                                                   \
        u##type remainder;                                                                                             \
        u##type result;                                                                                                \
    } divmod_##name##_t;                                                                                               \
    static divmod_##name##_t divmod_##name(u##type remainder, u##type divisor) {                                       \
        u##type const msb    = 1ull << (sizeof(type) * 8 - 1);                                                         \
        u##type       result = 0;                                                                                      \
        unsigned int  shift  = 0;                                                                                      \
        while (!(divisor & msb)) {                                                                                     \
            divisor <<= 1;                                                                                             \
            shift++;                                                                                                   \
        }                                                                                                              \
        for (unsigned int i = 0; i <= shift; i++) {                                                                    \
            if (remainder >= divisor) {                                                                                \
                result    |= 1ull << (shift - i);                                                                      \
                remainder -= divisor;                                                                                  \
            }                                                                                                          \
            divisor >>= 1;                                                                                             \
        }                                                                                                              \
        return (divmod_##name##_t){remainder, result};                                                                 \
    }

// Division implementations.
DIVMOD_IMPL(si_t, si)
DIVMOD_IMPL(di_t, di)
#ifdef do_ti_math
DIVMOD_IMPL(ti_t, ti)
#endif

// Generate all division functions for a type.
#define DIVMOD_FUNCS(type, name)                                                                                       \
    u##type __udivmod##name##4(u##type a, u##type b, u##type * rem) {                                                  \
        if (b == 0) {                                                                                                  \
            *rem = a;                                                                                                  \
            return -1;                                                                                                 \
        }                                                                                                              \
        divmod_##name##_t res = divmod_##name(a, b);                                                                   \
        *rem                  = res.remainder;                                                                         \
        return res.result;                                                                                             \
    }                                                                                                                  \
    u##type __udiv##name##3(u##type a, u##type b) {                                                                    \
        if (b == 0)                                                                                                    \
            return -1;                                                                                                 \
        return divmod_##name(a, b).result;                                                                             \
    }                                                                                                                  \
    u##type __umod##name##3(u##type a, u##type b) {                                                                    \
        if (b == 0)                                                                                                    \
            return a;                                                                                                  \
        else                                                                                                           \
            return divmod_##name(a, b).remainder;                                                                      \
    }                                                                                                                  \
    type __divmod##name##4(type a, type b, type * rem) {                                                               \
        if (b == 0) {                                                                                                  \
            *rem = a;                                                                                                  \
            return -1;                                                                                                 \
        }                                                                                                              \
        bool              div_sign = (a < 0) ^ (b < 0);                                                                \
        bool              mod_sign = a < b;                                                                            \
        divmod_##name##_t res      = divmod_##name(a, b);                                                              \
        *rem                       = mod_sign ? -res.remainder : res.remainder;                                        \
        return div_sign ? -res.result : res.result;                                                                    \
    }                                                                                                                  \
    type __div##name##3(type a, type b) {                                                                              \
        if (b == 0)                                                                                                    \
            return -1;                                                                                                 \
        bool sign = (a < 0) ^ (b < 0);                                                                                 \
        if (a < 0)                                                                                                     \
            a = -a;                                                                                                    \
        if (b < 0)                                                                                                     \
            b = -b;                                                                                                    \
        divmod_##name##_t res = divmod_##name(a, b);                                                                   \
        if (sign)                                                                                                      \
            return -res.result;                                                                                        \
        else                                                                                                           \
            return res.result;                                                                                         \
    }                                                                                                                  \
    type __mod##name##3(type a, type b) {                                                                              \
        if (b == 0)                                                                                                    \
            return a;                                                                                                  \
        bool sign = a < 0;                                                                                             \
        if (a < 0)                                                                                                     \
            a = -a;                                                                                                    \
        if (b < 0)                                                                                                     \
            b = -b;                                                                                                    \
        divmod_##name##_t res = divmod_##name(a, b);                                                                   \
        if (sign)                                                                                                      \
            return -res.remainder;                                                                                     \
        else                                                                                                           \
            return res.remainder;                                                                                      \
    }

DIVMOD_FUNCS(si_t, si)
DIVMOD_FUNCS(di_t, di)
#ifdef do_ti_math
DIVMOD_FUNCS(ti_t, ti)
#endif


// Fake integer operations by letting GCC write them all out itself.
// This file is O2 optimised, so it should emit assembly for all of these functions.
#define FAKE_OPER(type, name, oper)                                                                                    \
    type name(type a, type b) {                                                                                        \
        return a oper(b & (sizeof(type) * 8 - 1));                                                                     \
    }

FAKE_OPER(si_t, __ashlsi3, <<)
FAKE_OPER(si_t, __ashrsi3, >>)
FAKE_OPER(si_t, __lshrsi3, >>)
FAKE_OPER(di_t, __ashldi3, <<)
FAKE_OPER(di_t, __ashrdi3, >>)
FAKE_OPER(di_t, __lshrdi3, >>)
#ifdef do_ti_math
FAKE_OPER(ti_t, __ashlti3, <<)
FAKE_OPER(ti_t, __ashrti3, >>)
FAKE_OPER(ti_t, __lshrti3, >>)
#endif

// Explicit implementation of integer multiply for the larger versions to rely upon if required.
int __mulsi3(int a, int b) {
    int res = 0;
    for (unsigned int i = 0; i < sizeof(int) * 8; i++) {
        if (b & 1) {
            res += a;
        }
        b >>= 1;
        a <<= 1;
    }
    return res;
}

FAKE_OPER(di_t, __muldi3, *)
#ifdef do_ti_math
FAKE_OPER(ti_t, __multi3, *)
#endif

// The `__clz*` count leading zero functions count how many zeroes are present, starting at the MSB.
// They first convert a number into a bitmask where only bit above the most significant set bit is set.
// This is then multiplied with a de Bruijn sequence to get a unique index in the most significant bits.
// This index is then used to read from a hash table that uniquely identifies how many leading zeroes exist.

int __clzsi2(uint32_t a) __attribute__((weak));
int __clzsi2(uint32_t a) {
    static uint8_t const hash_table[32] = {
        0, 31, 9, 30, 3, 8,  13, 29, 2,  5,  7,  21, 12, 24, 28, 19,
        1, 10, 4, 14, 6, 22, 25, 20, 11, 15, 23, 26, 16, 27, 17, 18,
    };
    for (uint32_t i = 1; i < 32; i *= 2) {
        a |= a >> i;
    }
    a++;
    return hash_table[(a * 0x076be629) >> 27];
}
int __clzdi2(uint64_t a) __attribute__((weak));
int __clzdi2(uint64_t a) {
    static uint8_t const hash_table[64] = {
        0,  63, 62, 57, 61, 51, 56, 45, 60, 39, 50, 36, 55, 30, 44, 24, 59, 47, 38, 26, 49, 18,
        35, 16, 54, 33, 29, 10, 43, 14, 23, 7,  1,  58, 52, 46, 40, 37, 31, 25, 48, 27, 19, 17,
        34, 11, 15, 8,  2,  53, 41, 32, 28, 20, 12, 9,  3,  42, 21, 13, 4,  22, 5,  6,
    };
    for (uint64_t i = 1; i < 64; i *= 2) {
        a |= a >> i;
    }
    a++;
    return hash_table[(a * 0x0218a392cd3d5dbf) >> 58];
}
#ifdef do_ti_math
int __clzti2(__uint128_t a) {
    static uint8_t const hash_table[128] = {
        0,   127, 126, 120, 125, 113, 119, 106, 124, 99,  112, 92,  118, 85, 105, 78, 123, 95, 98,  71, 111, 64,
        91,  57,  117, 68,  84,  50,  104, 43,  77,  36,  122, 108, 94,  80, 97,  59, 70,  38, 110, 61, 63,  29,
        90,  27,  56,  22,  116, 88,  67,  46,  83,  25,  49,  15,  103, 54, 42,  12, 76,  20, 35,  8,  1,   121,
        114, 107, 100, 93,  86,  79,  96,  72,  65,  58,  69,  51,  44,  37, 109, 81, 60,  39, 62,  30, 28,  23,
        89,  47,  26,  16,  55,  13,  21,  9,   2,   115, 101, 87,  73,  66, 52,  45, 82,  40, 31,  24, 48,  17,
        14,  10,  3,   102, 74,  53,  41,  32,  18,  11,  4,   75,  33,  19, 5,   34, 6,   7,
    };
    for (__uint128_t i = 1; i < 128; i *= 2) {
        a |= a >> i;
    }
    a++;
    return hash_table[(a * 0x01061438916347932a5cd9d3ead7b77f) >> 121];
}
#endif

// NOLINTEND
