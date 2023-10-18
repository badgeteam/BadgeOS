# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

set(ESP32_HAL_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/spi_flash/esp32c6
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/spi_flash/include
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/spi_flash/include/esp_private
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/spi_flash/include/spi_flash
)

set(GLOB RECURSE ESP32_HAL_SOURCE_DIRECTORIES
    ${CMAKE_CURRENT_LIST_DIR}/esp-hal-3rdparty/components/spi_flash/*.c
)

target_sources(${target} PUBLIC ${ESP32_HAL_SOURCE_DIRECTORIES})

target_include_directories(${target} PUBLIC ${ESP32_HAL_INCLUDE_DIRECTORIES})