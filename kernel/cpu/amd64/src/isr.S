
# SPDX-License-Identifier: MIT

#include "cpu/isr.h"
#include "cpu/regs.h"
#include "port/hardware.h"
    .cfi_sections .debug_frame


    # Explicit context switch from M-mode.
    # Interrupts must be disabled on entry and will be re-enabled on exit.
    # If the context switch target is not set, this is a NOP.
    .text
    .global isr_context_switch
    .type isr_context_switch, %function
    .cfi_startproc
isr_context_switch:
    .cfi_endproc





    # Trap and system call handler.
    .text
    .type __trap_asm, %function
    .cfi_startproc
__trap_asm:
    .cfi_endproc





    # Interrupt handler.
    .text
    .type __isr_asm, %function
    .cfi_startproc
__isr_asm:
    .cfi_endproc
