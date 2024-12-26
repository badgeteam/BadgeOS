
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

set(cpu_riscv_enable_pmp true)
set(cpu_multicore true)

set(port_src
    ${CMAKE_CURRENT_LIST_DIR}/src/interrupt.c
    ${CMAKE_CURRENT_LIST_DIR}/src/memprotect.c
    ${CMAKE_CURRENT_LIST_DIR}/src/pmu_init.c
    ${CMAKE_CURRENT_LIST_DIR}/src/port.c
    ${CMAKE_CURRENT_LIST_DIR}/src/smp.c
)
set(port_include
    ${CMAKE_CURRENT_LIST_DIR}/include
)
set(port_link
    -T${CMAKE_CURRENT_LIST_DIR}/linker.ld
    -L${CMAKE_CURRENT_LIST_DIR}/../esp_common/esp-idf/components/soc/esp32p4/ld/
    -L${CMAKE_CURRENT_LIST_DIR}/../esp_common/esp-idf/components/esp_rom/esp32p4/ld/
)
set(port_flags
    -static -fno-pic
)

include(port/esp_common/CMakeLists.txt)
set_target_properties(${target} PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/linker.ld)
