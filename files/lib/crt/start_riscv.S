
# SPDX-License-Identifier: MIT

#include "syscall.h"

    .global __global_pointer$
    .global main



    .global _start
    .text
    .p2align 1
_start:
    # Set stack and global pointer.
    .option push
    .option norelax
    la sp, stack_top
    la gp, __global_pointer$
    .option pop
    
    # Set signal handler return trampoline.
    li a0, 0
    la a1, sigret_trampoline
    jal syscall_proc_sighandler
    
    # TODO: argc and argv
    
    # Jump to main function
    jal main
    
    # Exit the process if main returns
    li a7, SYSCALL_PROC_EXIT
    ecall



sigret_trampoline:
    j syscall_proc_sigret



    # Main stack.
    .section ".bss"
stack_bottom:
    .zero 1024
stack_top:
