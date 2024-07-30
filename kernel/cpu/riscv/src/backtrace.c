
// SPDX-License-Identifier: MIT

#include "backtrace.h"

#include "attributes.h"
#include "isr_ctx.h"
#include "rawprint.h"

#include <stddef.h>

#ifndef BACKTRACE_DEPTH
#define BACKTRACE_DEPTH 32
#endif



// Given stack frame pointer, perform backtrace.
void backtrace_from_ptr(void *frame_pointer) {
    rawprint("**** BEGIN BACKRTACE ****\n");
    // Prev FP offset: -2 words
    // Prev RA offset: -1 word
    size_t *fp = frame_pointer;
    for (int i = 0; i < BACKTRACE_DEPTH; i++) {
        size_t ra;
        if ((size_t)fp < 0x1000 || isr_noexc_copy_size(&ra, fp - 1)) {
            break;
        }
        rawprint("0x");
        rawprinthex(ra, sizeof(size_t) * 2);
        rawputc('\n');
        if ((size_t)fp < 0x1000 || isr_noexc_copy_size((size_t *)&fp, fp - 2)) {
            break;
        }
    }
    rawprint("**** END BACKRTACE ****\n");
}

// Perform backtrace as called.
void backtrace() NAKED;
#if __riscv_xlen == 64
void backtrace() {
    asm volatile("addi sp, sp, -16");
    asm volatile("sd   ra, 8(sp)");
    asm volatile("sd   s0, 0(sp)");
    asm volatile("addi s0, sp, 16");
    asm volatile("mv   a0, s0");
    asm volatile("jal  backtrace_from_ptr");
    asm volatile("ld   ra, 8(sp)");
    asm volatile("ld   s0, 0(sp)");
    asm volatile("addi sp, sp, 16");
    asm volatile("ret");
}
#else
void backtrace() {
    asm volatile("addi sp, sp, -16");
    asm volatile("sw   ra, 12(sp)");
    asm volatile("sw   s0, 8(sp)");
    asm volatile("addi s0, sp, 16");
    asm volatile("mv   a0, s0");
    asm volatile("jal  backtrace_from_ptr");
    asm volatile("lw   ra, 12(sp)");
    asm volatile("lw   s0, 8(sp)");
    asm volatile("addi sp, sp, 16");
    asm volatile("ret");
}
#endif
