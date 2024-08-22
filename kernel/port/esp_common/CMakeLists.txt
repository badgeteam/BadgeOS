
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

set(port_src ${port_src}
    ${CMAKE_CURRENT_LIST_DIR}/src/gpio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/i2c.c
    ${CMAKE_CURRENT_LIST_DIR}/src/time.c
)
set(port_include ${port_include}
    ${CMAKE_CURRENT_LIST_DIR}/include/
)
set(port_system_include ${port_system_include}
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/soc/${CONFIG_TARGET}/include/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/soc/include/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/hal/${CONFIG_TARGET}/include/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/hal/include/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/esp_rom/${CONFIG_TARGET}/include/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/esp_rom/${CONFIG_TARGET}/include/${CONFIG_TARGET}/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/esp_rom/include/
)
set(port_link ${port_link}
    -L${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/soc/${CONFIG_TARGET}/ld/
    -L${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/esp_rom/${CONFIG_TARGET}/ld/
)
set(create_esp32_bin true)
