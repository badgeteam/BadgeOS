
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

set(port_src ${port_src}
    ${CMAKE_CURRENT_LIST_DIR}/src/gpio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/i2c.c
    ${CMAKE_CURRENT_LIST_DIR}/src/time.c
)
set(port_include ${port_include}
    ${CMAKE_CURRENT_LIST_DIR}/include/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/soc/${BADGEROS_PORT}/include/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/soc/include/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/hal/${BADGEROS_PORT}/include/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/hal/include/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/esp_rom/include/${BADGEROS_PORT}/
    ${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/esp_rom/include/
)
set(port_link ${port_link}
    -L${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/soc/${BADGEROS_PORT}/ld/
    -L${CMAKE_CURRENT_LIST_DIR}/esp-idf/components/esp_rom/${BADGEROS_PORT}/ld/
)
