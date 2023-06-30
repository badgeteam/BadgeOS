
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

# Set the C compiler.
set(CMAKE_C_COMPILER riscv32-unknown-elf-gcc)
set(CMAKE_C_COMPILER_FORCED true)

set(common_compiler_flags
	-flto                              # Optimize the code at link time, on the whole program.
	-ffreestanding                     # We do not compile against an OS.
	-march=rv32imac_zifencei_zicsr     # Selects the target CPU.
	-mabi=ilp32 
	-nodefaultlibs                     # Do not link any default libraries like libgcc or libc.
	-O2                                # Optimize the code.
	-ggdb                              # Generate debug information in default extended format.
	-fsanitize=undefined               # Adds sanitizer for undefined behaviour.
	-fsanitize-undefined-trap-on-error # Invoke a trap instruction instead of calling into the UBsan runtime.
	-Werror=return-type                # Error when a function doesn't return a value, but declares to do so.
	-Wall -Wextra                      # Ramp up warning level.
)

# we must pass the same options to GCC and LD when using LTO, as the linker will actually do the codegen
add_compile_options(${common_compiler_flags})
add_link_options(${common_compiler_flags} -nostartfiles -T${CMAKE_CURRENT_LIST_DIR}/linker.ld)

# For IDE users.
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(badgeros C ASM)

# Define executable file.
set(target main.elf)
add_executable(${target}
	${CMAKE_CURRENT_LIST_DIR}/src/log.c
	${CMAKE_CURRENT_LIST_DIR}/src/main.c
)
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

# Include port-specific.
include(cpu/rv32imac/CMakeLists.txt)
include(port/esp32c6/CMakeLists.txt)
