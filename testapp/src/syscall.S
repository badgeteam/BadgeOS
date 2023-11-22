
# SPDX-License-Identifier: MIT

#include "syscall.h"



    .global sys_temp_write
    .text
    .p2align 1
sys_temp_write:
    li a7, SYSCALL_TEMP_WRITE
    ecall
    ret
