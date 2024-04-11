
# SPDX-License-Identifier: MIT



    # Run a restricted function and catch exceptions.
    .section ".text.isr_noexc_run"
    .global isr_noexc_run
    .type isr_noexc_run, %function
isr_noexc_run:
    # Save execution state.
    addi  sp, sp, -32
    sw    a2, 0(sp)         # cookie
    sw    a1, 4(sp)         # trap handler
    li    t0, 0xf
    csrrc t0, mstatus, t0
    sw    t0, 8(sp)         # mstatus
    csrrw t0, mscratch, sp
    sw    t0, 12(sp)        # mscratch
    la    t0, isr_noexc_vec
    csrrw t0, mtvec, t0
    sw    t0, 16(sp)        # mtvec
    sw    ra, 20(sp)        # return address
    sw    x0, 24(sp)        # has trapped
    # Run the noexc function.
    mv    ra, a0            # function to call
    mv    a0, a2            # cookie
    jalr  ra
isr_noexc_bail:
    # Restore execution state.
    lw    ra, 20(sp)        # return address
    lw    t0, 12(sp)        # mscratch
    csrw  mscratch, t0
    lw    t0, 16(sp)        # mtvec
    csrw  mtvec, t0
    lw    t0, 8(sp)         # mstatus
    csrw  mstatus, t0
    lw    a0, 24(sp)        # has trapped
    addi  sp, sp, 32
    ret



    # Temporary trap handler for noexc runtime.
isr_noexc_tvec:
    addi  sp, sp, -128
    # Preserve registers.
#pragma region
    sw    x1,  4(sp)
    sw    x2,  8(sp)
    sw    x3,  12(sp)
    sw    x4,  16(sp)
    sw    x5,  20(sp)
    sw    x6,  24(sp)
    sw    x7,  28(sp)
    sw    x8,  32(sp)
    sw    x9,  36(sp)
    sw    x10, 40(sp)
    sw    x11, 44(sp)
    sw    x12, 48(sp)
    sw    x13, 52(sp)
    sw    x14, 56(sp)
    sw    x15, 60(sp)
    sw    x16, 64(sp)
    sw    x17, 68(sp)
    sw    x18, 72(sp)
    sw    x19, 76(sp)
    sw    x20, 80(sp)
    sw    x21, 84(sp)
    sw    x22, 88(sp)
    sw    x23, 92(sp)
    sw    x24, 96(sp)
    sw    x25, 100(sp)
    sw    x26, 104(sp)
    sw    x27, 108(sp)
    sw    x28, 112(sp)
    sw    x29, 116(sp)
    sw    x30, 120(sp)
    sw    x31, 124(sp)
#pragma endregion
    # Save PC.
    csrr  t0, mepc
    sw    t0, 0(sp)
    # Get cookie.
    csrr  t0, mscratch
    lw    a0, 0(t0)
    # Generate register pointer.
    mv    a1, sp
    # Mark as trapped.
    li    t1, 1
    sw    t1, 24(t0)
    # Call Le ISR.
    lw    ra, 4(t0)
    beqz  ra, isr_noexc_tvec_none
    jalr  ra
isr_noexc_tvec_none:
    # Retour to the noexc wrapper.
    csrr  sp, mscratch
    j     isr_noexc_bail



    # Temporary interrupt handler for noexc runtime.
isr_noexc_ivec:
    ebreak



    # Interrupt vector table for the CPU.
    # This must be aligned to a 256-byte boundary, so it is in a special section.
    .section ".interrupt_vector_table"
isr_noexc_vec:
    .option push
    .option norvc
    # Trap handler.
    j isr_noexc_tvec
    # Interrupt handler.
    .rept 31
    .word isr_noexc_ivec
    .endr
    # Padding to 256-byte size.
    .skip 128
    .option pop