
# SPDX-License-Identifier: MIT

#include "cpu/regs.h"
    .global kernel_reg_dump_arr



    # Print a register dump of the current registers.
    .global kernel_cur_regs_dump
    .type kernel_cur_regs_dump, %function
kernel_cur_regs_dump:
    ret



    # Calls the actual trapping code in `isr_noexc_run`.
    .global _isr_noexc_run_int
    .type _isr_noexc_run_int, %function
_isr_noexc_run_int:
    // TODO.
