
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# Set the C compiler.
if(NOT DEFINED CMAKE_C_COMPILER)
    find_program(CMAKE_C_COMPILER NAMES riscv32-unknown-linux-gnu-gcc riscv32-linux-gnu-gcc riscv64-unknown-linux-gnu-gcc riscv64-linux-gnu-gcc REQUIRED)
    message("Detected RISC-V C compiler as '${CMAKE_C_COMPILER}'")
else()
    message("Using compiler '${CMAKE_C_COMPILER}' from environment")
endif()

# determine the compiler prefix
get_filename_component(compiler_name "${CMAKE_C_COMPILER}" NAME)
string(REGEX MATCH "^([A-Za-z0-9_]+\-)*" BADGER_COMPILER_PREFIX "${compiler_name}") 
find_program(BADGER_OBJCOPY NAMES "${BADGER_COMPILER_PREFIX}objcopy" REQUIRED)  
find_program(BADGER_OBJDUMP NAMES "${BADGER_COMPILER_PREFIX}objdump" REQUIRED)

set(target_arch rv32imac_zifencei_zicsr)
if(DEFINED TARGET_ARCH)
	set(target_arch "${TARGET_ARCH}")
endif()

set(target_abi ilp32)
if(DEFINED TARGET_ABI)
	set(target_abi "${TARGET_ABI}")
endif()

# LTO is disabled due to GCC bugs inserting calls to memcpy everywhere
set(common_compiler_flags
    -ffreestanding                     # We do not compile against an OS.
    -march=${target_arch}              # Selects the target CPU.
    -mabi=${target_abi}                # Selects target ABI 
    -nodefaultlibs                     # Do not link any default libraries like libgcc or libc.
    -fno-tree-loop-distribute-patterns
    -O1                                # Optimize the code.
    -ggdb                              # Generate debug information in default extended format.
    -fsanitize=undefined               # Adds sanitizer for undefined behaviour.
    -fsanitize-undefined-trap-on-error # Invoke a trap instruction instead of calling into the UBsan runtime.
    -Werror=return-type                # Error when a function doesn't return a value, but declares to do so.
    -Wall -Wextra                      # Ramp up warning level.
    -std=gnu11                         # We use the C11 standard
    -fno-tree-loop-distribute-patterns # do not generate hidden calls to memset/memcpy
    -fpie -pie                         # Generate position-independent code
    -fno-omit-frame-pointer            # Always use frame pointer
)

# we must pass the same options to GCC and LD when using LTO, as the linker will actually do the codegen
add_compile_options(${common_compiler_flags})
add_link_options(${common_compiler_flags} -nostartfiles)

# For IDE users.
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(badgeros C ASM)

# Define executable file.
set(target app.elf)
add_executable(${target}
    ${CMAKE_CURRENT_LIST_DIR}/src/start.S
    ${CMAKE_CURRENT_LIST_DIR}/src/main.c
    ${CMAKE_CURRENT_LIST_DIR}/src/syscall.c
)
target_include_directories(${target} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/../../common
)

# Declare which files are installed to the output directory:
install(TARGETS app.elf DESTINATION .)
