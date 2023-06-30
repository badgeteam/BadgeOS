
// SPDX-License-Identifier: MIT

#pragma once

// this file provides convenience macros for the attributes provided by gcc:
// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html

// Declares that a function cannot return.
#define NORETURN __attribute__((noreturn))

// Declares that a function has no observable side effects and does not mutate
// its parameters.
#define PURE __attribute__((const))

// Declares that a function is not called very often and can be size-optimized
// even in fast builds.
#define COLD __attribute__((cold))

// Declares that a function call must be inlined whenever possible.
#define FORCEINLINE __attribute__((always_inline))
