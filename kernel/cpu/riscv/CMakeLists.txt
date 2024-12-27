
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

set(cpu_src
    ${CMAKE_CURRENT_LIST_DIR}/src/entrypoint.S
    ${CMAKE_CURRENT_LIST_DIR}/src/isr.S
    ${CMAKE_CURRENT_LIST_DIR}/src/isr_ctx.S
    ${CMAKE_CURRENT_LIST_DIR}/src/syscall.S

    ${CMAKE_CURRENT_LIST_DIR}/src/backtrace.c
    ${CMAKE_CURRENT_LIST_DIR}/src/isr.c
    ${CMAKE_CURRENT_LIST_DIR}/src/isr_ctx.c
    ${CMAKE_CURRENT_LIST_DIR}/src/panic.c
    ${CMAKE_CURRENT_LIST_DIR}/src/scheduler.c
    ${CMAKE_CURRENT_LIST_DIR}/src/usercopy.c
)
set(cpu_include ${CMAKE_CURRENT_LIST_DIR}/include)
set(cpu_link)
set(cpu_flags)

if(DEFINED cpu_multicore)
    set(cpu_src ${cpu_src} ${CMAKE_CURRENT_LIST_DIR}/src/cpu1_entrypoint.S)
endif()
if(DEFINED cpu_riscv_enable_riscv_intc)
    set(cpu_src ${cpu_src} ${CMAKE_CURRENT_LIST_DIR}/src/interrupt/riscv_intc.c)
    set(cpu_src ${cpu_src} ${CMAKE_CURRENT_LIST_DIR}/src/interrupt.c)
endif()
if(DEFINED cpu_riscv_enable_riscv_plic)
    set(cpu_src ${cpu_src} ${CMAKE_CURRENT_LIST_DIR}/src/interrupt/riscv_plic.c)
    set(cpu_src ${cpu_src} ${CMAKE_CURRENT_LIST_DIR}/src/interrupt.c)
endif()
if(DEFINED cpu_riscv_enable_pmp)
    set(cpu_src ${cpu_src} ${CMAKE_CURRENT_LIST_DIR}/src/memprotect/riscv_pmp.c)
endif()
if(DEFINED cpu_riscv_enable_mmu)
    set(cpu_src ${cpu_src} ${CMAKE_CURRENT_LIST_DIR}/src/memprotect/mmu.c)
endif()
if(DEFINED cpu_riscv_enable_sbi_time)
    set(cpu_src ${cpu_src} ${CMAKE_CURRENT_LIST_DIR}/src/sbi_time.c)
    add_definitions(-DCPU_RISCV_ENABLE_SBI_TIME)
endif()
