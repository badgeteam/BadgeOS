
# SPDX-License-Identifier: MIT



    .text
    .align 2
    .global amd64_syscall_wrapper
    # Called directly from `sched_raise_from_isr`; overwrites callee-save registers.
amd64_syscall_wrapper:
