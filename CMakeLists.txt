
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
    -DBADGEROS_KERNEL                  # Tell the code we're building for the kernel
    -DBADGEROS_MALLOC_DEBUG_LEVEL=4    # Turn on all malloc() debug stuff for now. 
)

# we must pass the same options to GCC and LD when using LTO, as the linker will actually do the codegen
add_compile_options(${common_compiler_flags})
add_link_options(${common_compiler_flags} -nostartfiles -T${CMAKE_CURRENT_LIST_DIR}/linker.ld)

# For IDE users.
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(badgeros C ASM)

# Define executable file.
set(target badger-os.elf)
add_executable(${target}
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/arrays.c
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/assertions.c
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/badge_format_str.c
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/badge_strings.c
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/log.c
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/mutex.c
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/num_to_str.c
    ${CMAKE_CURRENT_LIST_DIR}/src/blockdevice/blkdev_ram.c
    ${CMAKE_CURRENT_LIST_DIR}/src/blockdevice/blockdevice.c
    ${CMAKE_CURRENT_LIST_DIR}/src/filesystem/filesystem.c
    # ${CMAKE_CURRENT_LIST_DIR}/src/filesystem/vfs_fat.c
    ${CMAKE_CURRENT_LIST_DIR}/src/filesystem/vfs_ramfs.c
    ${CMAKE_CURRENT_LIST_DIR}/src/filesystem/vfs_internal.c
    ${CMAKE_CURRENT_LIST_DIR}/src/loading/kbelfx.c
    ${CMAKE_CURRENT_LIST_DIR}/src/loading/rom.c
    ${CMAKE_CURRENT_LIST_DIR}/src/process/process.c
    ${CMAKE_CURRENT_LIST_DIR}/src/process/syscall_impl.c
    ${CMAKE_CURRENT_LIST_DIR}/src/list.c
    ${CMAKE_CURRENT_LIST_DIR}/src/main.c
    ${CMAKE_CURRENT_LIST_DIR}/src/malloc/malloc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/scheduler.c
    ${CMAKE_CURRENT_LIST_DIR}/src/syscall.c
)
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include ${CMAKE_CURRENT_LIST_DIR}/include/badgelib)

# Include port-specific.
include(cpu/rv32imac/CMakeLists.txt)
include(port/esp32c6/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/CmakeEspHal3rdParty.cmake)

# Add libraries.
add_compile_options(-I${CMAKE_CURRENT_LIST_DIR}/include -I${CMAKE_CURRENT_LIST_DIR}/include/badgelib -include ${CMAKE_CURRENT_LIST_DIR}/include/loading/kbelf_pre.h)
add_subdirectory(lib/kbelf)
target_link_libraries(${target} PUBLIC kbelf)


# Convert the raw binary file into a ESP32 image file
add_custom_command(
    OUTPUT badger-os.bin
    COMMAND cp badger-os.elf badger-os.elf.patch
    COMMAND "${BADGER_OBJCOPY}" -O binary badger-os.elf badger-os.nochecksum.bin
    COMMAND python3 "${CMAKE_CURRENT_LIST_DIR}/tools/pack-image.py" badger-os.nochecksum.bin badger-os.bin
    DEPENDS badger-os.elf
)
# Proxy target to allow installation of the bin file
add_custom_target(badger-os.bin.target ALL DEPENDS badger-os.bin)

# Also build a dump file that is put into the 
add_custom_target(
    badger-os.elf.disasm
    ALL
    COMMAND "${BADGER_OBJDUMP}" -Sd badger-os.elf > badger-os.elf.disasm
    DEPENDS badger-os.elf
)

# Declare which files are installed to the output directory:
install(TARGETS badger-os.elf DESTINATION .)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/badger-os.bin" DESTINATION .)

