
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

set(cpu_riscv_enable_riscv_intc true)
set(cpu_riscv_enable_riscv_plic true)
set(cpu_riscv_enable_sbi_time true)
set(cpu_riscv_enable_mmu true)

set(port_src
    ${CMAKE_CURRENT_LIST_DIR}/src/driver/pcie.c
    ${CMAKE_CURRENT_LIST_DIR}/src/hal/gpio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/hal/i2c.c
    ${CMAKE_CURRENT_LIST_DIR}/src/hal/spi.c
    ${CMAKE_CURRENT_LIST_DIR}/src/memprotect.c
    ${CMAKE_CURRENT_LIST_DIR}/src/port.c
    ${CMAKE_CURRENT_LIST_DIR}/src/smp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/uacpi_kernel_api.c
)
set(port_include
    ${CMAKE_CURRENT_LIST_DIR}/include
    lib/limine
)
set(port_link
    -T${CMAKE_CURRENT_LIST_DIR}/linker_${CONFIG_CPU}.ld
    -L${CMAKE_CURRENT_LIST_DIR}
)
set(port_flags)
set_target_properties(${target} PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/linker.ld)

if(${CONFIG_CPU} STREQUAL "amd64")
    include(lib/uacpi/uacpi.cmake)
    set(port_src ${port_src} ${UACPI_SOURCES})
    set(port_include ${port_include} ${UACPI_INCLUDES})
else()
    set(port_flags ${port_flags} -DPORT_ENABLE_DTB)
    set(port_src ${port_src} 
        ${CMAKE_CURRENT_LIST_DIR}/src/dtb.c
        ${CMAKE_CURRENT_LIST_DIR}/src/dtparse.c
    )
endif()
