
// SPDX-License-Identifier: MIT

#include "syscall.h"

// These syscalls *do* actually take parameters, but GCC doesn't realise this.
#pragma GCC diagnostic ignored "-Wunused-parameter"

#define SYSCALL_DEF(num, enum, name, returns, ...)                                                                     \
    returns name(__VA_ARGS__) __attribute__((naked));                                                                  \
    returns name(__VA_ARGS__) {                                                                                        \
        asm("li a7, " #num "; ecall; ret");                                                                            \
    }

#include "syscall_defs.inc"
