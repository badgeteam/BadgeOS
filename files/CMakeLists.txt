
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)
include(../common/compiler.cmake)

if("${BADGEROS_PORT}" STREQUAL "esp32c6")
    set(target_arch rv32imac_zicsr_zifencei)
    set(target_abi ilp32)
elseif("${BADGEROS_PORT}" STREQUAL "esp32p4")
    set(target_arch rv32imafc_zicsr_zifencei)
    set(target_abi ilp32)
endif()

set(badge_cflags
    -ffreestanding
    -fpie
    -march=${target_arch}              # Selects the target CPU.
    -mabi=${target_abi}                # Selects target ABI
    -nodefaultlibs -nostdlib           # Do not link any default libraries like libgcc or libc.
    -O2                                # Optimize the code.
    -Werror=return-type                # Error when a function doesn't return a value, but declares to do so.
    -Werror=implicit-fallthrough
    -Wall -Wextra                      # Ramp up warning level.
    -std=gnu11                         # We use the C11 standard
    -DBADGEROS                         # Tell the code we're building for the userspace
    -DBADGEROS_PORT_${BADGEROS_PORT}
    -DBADGEROS_PORT="${BADGEROS_PORT}"
    -fno-omit-frame-pointer            # Always use frame pointer
    -fno-stack-protector
    -fno-exceptions
)

project(root C ASM)

set(badge_include
    ${CMAKE_CURRENT_LIST_DIR}/../common/include
    ${CMAKE_CURRENT_LIST_DIR}/../common/badgelib
)
set(badge_libs crt)
macro(badgeros_executable exec installdir)
    add_executable(${exec})
    target_compile_options(${exec} PRIVATE ${badge_cflags} -ffunction-sections)
    target_link_options(${exec} PRIVATE ${badge_cflags} -Wl,--gc-sections -nostartfiles)
    target_include_directories(${exec} PRIVATE ${badge_include})
    target_link_libraries(${exec} PRIVATE ${badge_libs})
    install(TARGETS ${exec} RUNTIME DESTINATION ${installdir})
endmacro()

add_subdirectory(lib)
add_subdirectory(sbin)
