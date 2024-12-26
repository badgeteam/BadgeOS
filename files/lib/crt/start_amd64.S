
# SPDX-License-Identifier: MIT

#include "syscall.h"

    .global main



    .global _start
    .text
    .p2align 1
_start:
    // TODO.



sigret_trampoline:
    jmp syscall_proc_sigret



    # Main stack.
    .section ".bss"
stack_bottom:
    .zero 1024
stack_top:
