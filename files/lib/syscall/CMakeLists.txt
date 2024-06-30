
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

add_library(syscall
    src/gpio.c
    src/i2c.c
    src/spi.c
    src/syscall.c
)
target_compile_options(syscall PRIVATE ${badge_cflags} -ffunction-sections)
target_link_options(syscall PRIVATE ${badge_cflags} -Wl,--gc-sections -nostartfiles)
target_include_directories(syscall PRIVATE ${badge_include})
