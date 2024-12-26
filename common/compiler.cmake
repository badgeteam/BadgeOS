
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

# Determine the compiler prefix
set(CMAKE_C_COMPILER "${CONFIG_COMPILER}")
set(BADGER_OBJCOPY "${CONFIG_TC_PREFIX}objcopy")
set(BADGER_OBJDUMP "${CONFIG_TC_PREFIX}objdump")

# Determine arch options
if("${CONFIG_CPU}" STREQUAL "riscv32")
    set(target_arch_prefix rv32)
    set(target_abi ilp32)
elseif("${CONFIG_CPU}" STREQUAL "riscv64")
    set(target_arch_prefix rv64)
    set(target_abi lp64)
else()
    set(target_arch_prefix amd64)
    set(target_abi sysv)
endif()

if("${CONFIG_CPU}" STREQUAL "amd64")
    set(target_arch "x86-64")
else()
    if("${CONFIG_FLOAT_SPEC}" STREQUAL "single")
        set(target_float_frac f)
    elseif("${CONFIG_FLOAT_SPEC}" STREQUAL "double")
        set(target_float_frac fd)
    else()
        set(target_float_frac)
    endif()

    set(target_arch "${target_arch_prefix}ima${target_float_frac}c_zicsr_zifencei")
endif()