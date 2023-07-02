
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

target_sources(${target} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src/int64.S
    ${CMAKE_CURRENT_LIST_DIR}/src/isr.S
    ${CMAKE_CURRENT_LIST_DIR}/src/kernel_ctx.S

    ${CMAKE_CURRENT_LIST_DIR}/src/isr.c
    ${CMAKE_CURRENT_LIST_DIR}/src/kernel_ctx.c
    ${CMAKE_CURRENT_LIST_DIR}/src/panic.c
    ${CMAKE_CURRENT_LIST_DIR}/src/scheduler.c
)
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
