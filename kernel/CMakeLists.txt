
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13.0)
cmake_policy(VERSION 3.13.0)

# For IDE users.
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# Set the RAMFS root
if(NOT DEFINED BADGER_RAMFS_ROOT)
    set(BADGER_RAMFS_ROOT ${CMAKE_CURRENT_LIST_DIR}/root)
endif()

# Include the config.
include(../.config/config.cmake)
include(../common/compiler.cmake)

# Include port-specific.
include(port/${CONFIG_TARGET}/CMakeLists.txt)
include(cpu/${CONFIG_CPU}/CMakeLists.txt)

set(common_compiler_flags
    -ffreestanding                     # We do not compile against an OS.
    -march=${target_arch}              # Selects the target CPU.
    -mabi=${target_abi}                # Selects target ABI
    -nodefaultlibs                     # Do not link any default libraries like libgcc or libc.
    -O0                                # Optimize the code.
    -ggdb                              # Generate debug information in default extended format.
    -Werror=return-type                # Error when a function doesn't return a value, but declares to do so.
    -Werror=implicit-fallthrough
    -Werror=int-conversion
    -Wall -Wextra                      # Ramp up warning level.
    -Wno-missing-braces
    -std=gnu11                         # We use the C11 standard
    -DBADGEROS_KERNEL                  # Tell the code we're building for the kernel
    -DBADGEROS_MALLOC_DEBUG_LEVEL=2    # Malloc debug level set to WARN
    -DSOFTBIT                          # Turn on our emulated bit operations
    -fno-omit-frame-pointer            # Always use frame pointer
    -ffunction-sections
    -fno-stack-protector
    -fno-exceptions
)
if("${CMAKE_BUILD_TYPE}" MATCHES "Release")
message("Building in release mode")
else()
message("Building in debug mode")
endif()

# we must pass the same options to GCC and LD when using LTO, as the linker will actually do the codegen
add_compile_options(${common_compiler_flags} ${cpu_flags} ${port_flags})
add_link_options(
    ${common_compiler_flags} ${cpu_flags} ${port_flags}
    ${cpu_link} ${port_link}
    -Wl,--gc-sections -Wl,--build-id=none -nodefaultlibs -nostartfiles
)

project(badgeros C ASM)

# Define executable file.
set(target badger-os.elf)
add_executable(${target}
    ${CMAKE_CURRENT_LIST_DIR}/build/fs_root.c
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/badge_format_str.c
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/log.c
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/mutex.c
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/num_to_str.c
    ${CMAKE_CURRENT_LIST_DIR}/src/badgelib/rawprint.c
    
    ${CMAKE_CURRENT_LIST_DIR}/src/blockdevice/blkdev_ram.c
    ${CMAKE_CURRENT_LIST_DIR}/src/blockdevice/blockdevice.c
    
    ${CMAKE_CURRENT_LIST_DIR}/src/filesystem/filesystem.c
    ${CMAKE_CURRENT_LIST_DIR}/src/filesystem/syscall_impl.c
    # ${CMAKE_CURRENT_LIST_DIR}/src/filesystem/vfs_fat.c
    ${CMAKE_CURRENT_LIST_DIR}/src/filesystem/vfs_ramfs.c
    ${CMAKE_CURRENT_LIST_DIR}/src/filesystem/vfs_internal.c
    ${CMAKE_CURRENT_LIST_DIR}/src/freestanding/int_routines.c
    ${CMAKE_CURRENT_LIST_DIR}/src/freestanding/string.c
    ${CMAKE_CURRENT_LIST_DIR}/src/hal/syscall_impl.c
    
    ${CMAKE_CURRENT_LIST_DIR}/src/malloc/malloc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/malloc/static-buddy.c
    ${CMAKE_CURRENT_LIST_DIR}/src/malloc/slab-alloc.c
    
    ${CMAKE_CURRENT_LIST_DIR}/src/process/kbelfx.c
    ${CMAKE_CURRENT_LIST_DIR}/src/process/proc_memmap.c
    ${CMAKE_CURRENT_LIST_DIR}/src/process/process.c
    ${CMAKE_CURRENT_LIST_DIR}/src/process/sighandler.c
    ${CMAKE_CURRENT_LIST_DIR}/src/process/syscall_impl.c
    ${CMAKE_CURRENT_LIST_DIR}/src/process/syscall_util.c
    
    ${CMAKE_CURRENT_LIST_DIR}/src/scheduler/scheduler.c
    ${CMAKE_CURRENT_LIST_DIR}/src/scheduler/syscall_impl.c
    
    ${CMAKE_CURRENT_LIST_DIR}/src/housekeeping.c
    ${CMAKE_CURRENT_LIST_DIR}/src/main.c
    ${CMAKE_CURRENT_LIST_DIR}/src/page_alloc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/syscall.c
    
    ${cpu_src}
    ${port_src}
)
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src/malloc
    ${CMAKE_CURRENT_LIST_DIR}/include/badgelib
    ${CMAKE_CURRENT_LIST_DIR}/../common/include
    ${CMAKE_CURRENT_LIST_DIR}/../common/badgelib
    ${CMAKE_CURRENT_LIST_DIR}/../.config
    ${cpu_include}
    ${port_include}
)
include_directories(SYSTEM ${port_system_include})

# Add libraries.
add_subdirectory(../common/badgelib badgelib)
add_subdirectory(lib/kbelf)
target_compile_options(kbelf PRIVATE
    -include ${CMAKE_CURRENT_LIST_DIR}/include/loading/kbelf_pre.h
)
target_link_libraries(kbelf PRIVATE badgelib)
target_link_libraries(${target} PRIVATE kbelf badgelib)


# Copy the BadgerOS binary to the output directory.
install(TARGETS badger-os.elf DESTINATION .)

# Create a disassembled version for easier debugging.
add_custom_target(
    badger-os.elf.disasm
    ALL
    COMMAND ${BADGER_OBJDUMP} -Sd badger-os.elf > badger-os.elf.disasm
    DEPENDS badger-os.elf
)

if(${create_esp32_bin})
    # Convert the raw binary file into a ESP32 image file
    add_custom_command(
        OUTPUT badger-os.bin
        COMMAND cp badger-os.elf badger-os.elf.patch
        COMMAND ${BADGER_OBJCOPY} -O binary badger-os.elf badger-os.nochecksum.bin
        COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/../tools/pack-image.py badger-os.nochecksum.bin badger-os.bin
        DEPENDS badger-os.elf
    )

    # Install the ESP32 image.
    add_custom_target(badger-os.bin.target ALL DEPENDS badger-os.bin)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/badger-os.bin DESTINATION .)
endif()
