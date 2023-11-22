# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

set(ESP32_HAL_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/hal/esp32c6/include
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/esp_common/include
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/spi_flash/sim/sdkconfig
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/soc/include
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/soc/esp32c6/include
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/esp_rom/include
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/hal/platform_port/include
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/hal/include
)

set(GLOB RECURSE ESP32_HAL_SOURCE_DIRECTORIES
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/spi_flash/*.c
)

target_sources(${target} PUBLIC ${ESP32_HAL_SOURCE_DIRECTORIES})

target_include_directories(${target} PUBLIC ${ESP32_HAL_INCLUDE_DIRECTORIES})