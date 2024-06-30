
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

# The target platform
if(NOT DEFINED ENV{BADGEROS_PORT})
    set(BADGEROS_PORT esp32c6)
else()
    set(BADGEROS_PORT $ENV{BADGEROS_PORT})
endif()

# Set the C compiler
if(DEFINED CMAKE_C_COMPILER)
    message("Using compiler '${CMAKE_C_COMPILER}' from environment")
elseif(DEFINED ENV{CMAKE_C_COMPILER})
    set(CMAKE_C_COMPILER $ENV{CMAKE_C_COMPILER})
    message("Using compiler '${CMAKE_C_COMPILER}' from environment")
else()
    find_program(CMAKE_C_COMPILER NAMES riscv32-badgeros-linux-gnu-gcc riscv32-unknown-linux-gnu-gcc riscv32-linux-gnu-gcc riscv64-unknown-linux-gnu-gcc riscv64-linux-gnu-gcc REQUIRED)
    message("Detected RISC-V C compiler as '${CMAKE_C_COMPILER}'")
endif()

# Determine the compiler prefix
get_filename_component(compiler_name "${CMAKE_C_COMPILER}" NAME)
string(REGEX MATCH "^([A-Za-z0-9_]+\-)*" BADGER_COMPILER_PREFIX "${compiler_name}")
find_program(BADGER_OBJCOPY NAMES "${BADGER_COMPILER_PREFIX}objcopy" REQUIRED)
find_program(BADGER_OBJDUMP NAMES "${BADGER_COMPILER_PREFIX}objdump" REQUIRED)
