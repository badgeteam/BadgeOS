
# SPDX-License-Identifier: MIT



    .text
    .align 2
    .global riscv_syscall_wrapper
    # Called directly from `sched_raise_from_isr`; overwrites callee-save registers.
riscv_syscall_wrapper:
    # Save arguments.
    mv   s1, a0
    mv   s2, a1
    mv   s3, a2
    mv   s4, a3
    mv   s5, a4
    mv   s6, a5
    mv   s7, a6
    # Get syscall information.
    mv   a0, a7
    jal  syscall_info
    # If it doesn't exist, go to sigsys handler.
    bnez a0, .gotosys
    j    proc_sigsys_handler
    # Run the system call.
.gotosys:
    mv   s8, a0
    mv   s9, a1
    mv   a0, s1
    mv   a1, s2
    mv   a2, s3
    mv   a3, s4
    mv   a4, s5
    mv   a5, s6
    mv   a6, s7
    jalr s8
    # Truncate return value.
    andi t0, s9, 255
    li   t1, __riscv_xlen/8
    bgt  t0, t1, .args2
    bnez t0, .args1
    li   a0, 0
.args1:
    li   a1, 0
.args2:
    # Return from system call.
    j    syscall_return
