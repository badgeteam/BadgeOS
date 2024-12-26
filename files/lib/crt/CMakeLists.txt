
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

if(${CONFIG_CPU} STREQUAL "amd64")
    add_library(crt start_amd64.S)
else()
    add_library(crt start_riscv.S)
endif()
target_compile_options(crt PRIVATE ${badge_cflags} -ffunction-sections)
target_link_options(crt PRIVATE ${badge_cflags} -Wl,--gc-sections -nostartfiles)
target_include_directories(crt PRIVATE ${badge_include})
target_link_libraries(crt PUBLIC syscall)
