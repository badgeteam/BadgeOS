
// SPDX-License-Identifier: MIT
// Port of hal/regi2c_ctrl.h

#pragma once

#define REGI2C_WRITE(x, y, z)                                                                                          \
    do {                                                                                                               \
        __builtin_trap();                                                                                              \
        (void)(x);                                                                                                     \
        (void)(y);                                                                                                     \
        (void)(z);                                                                                                     \
    } while (0)
#define REGI2C_WRITE_MASK(x, y, z)                                                                                     \
    do {                                                                                                               \
        __builtin_trap();                                                                                              \
        (void)(x);                                                                                                     \
        (void)(y);                                                                                                     \
        (void)(z);                                                                                                     \
    } while (0)
