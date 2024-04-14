
# SPDX-License-Identifier: MIT

#include "syscall.h"



    .text
    .p2align 1

    .global syscall_temp_write
syscall_temp_write:
    li a7, SYSCALL_TEMP_WRITE
    ecall
    ret

    .global syscall_fs_open
syscall_fs_open:
    li a7, SYSCALL_FS_OPEN
    ecall
    ret

    .global syscall_fs_close
syscall_fs_close:
    li a7, SYSCALL_FS_CLOSE
    ecall
    ret

    .global syscall_fs_read
syscall_fs_read:
    li a7, SYSCALL_FS_READ
    ecall
    ret

    .global syscall_fs_write
syscall_fs_write:
    li a7, SYSCALL_FS_WRITE
    ecall
    ret

    .global syscall_fs_getdents
syscall_fs_getdents:
    li a7, SYSCALL_FS_GETDENTS
    ecall
    ret

    .global syscall_mem_alloc
syscall_mem_alloc:
    li a7, SYSCALL_MEM_ALLOC
    ecall
    ret

    .global syscall_mem_dealloc
syscall_mem_dealloc:
    li a7, SYSCALL_MEM_DEALLOC
    ecall
    ret
