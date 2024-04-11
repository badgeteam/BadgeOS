
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

set(target_arch "rv32imafc_zicsr_zifencei")
set(target_abi "ilp32")

set(port_src
    # ${CMAKE_CURRENT_LIST_DIR}/src/hal/gpio.c
    # ${CMAKE_CURRENT_LIST_DIR}/src/hal/i2c.c
    
    # ${CMAKE_CURRENT_LIST_DIR}/src/clkconfig.c
    ${CMAKE_CURRENT_LIST_DIR}/src/interrupt.c
    ${CMAKE_CURRENT_LIST_DIR}/src/memprotect.c
    ${CMAKE_CURRENT_LIST_DIR}/src/pmu_init.c
    ${CMAKE_CURRENT_LIST_DIR}/src/port.c
)
set(port_include
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/../esp_common/include
    ${CMAKE_CURRENT_LIST_DIR}/../esp_common/esp-idf/components/soc/esp32p4/include
    ${CMAKE_CURRENT_LIST_DIR}/../esp_common/esp-idf/components/soc/include
    ${CMAKE_CURRENT_LIST_DIR}/../esp_common/esp-idf/components/esp_rom/include/esp32p4/
)
set(port_link
    -T${CMAKE_CURRENT_LIST_DIR}/linker.ld
    -L${CMAKE_CURRENT_LIST_DIR}/../esp_common/esp-idf/components/soc/esp32p4/ld/
    -L${CMAKE_CURRENT_LIST_DIR}/../esp_common/esp-idf/components/esp_rom/esp32p4/ld/
)
set(port_flags)

include(port/esp_common/CMakeLists.txt)
