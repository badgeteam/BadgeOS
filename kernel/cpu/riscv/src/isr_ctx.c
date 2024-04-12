
// SPDX-License-Identifier: MIT

#include "isr_ctx.h"

#include "rawprint.h"

// Number of columns in register dump.
// Must be a power of 2.
#define COLS 4

// Register names table.
static char const regnames[32][4] = {
    "PC ", "RA ", "SP ", "GP ", "TP ", "T0 ", "T1 ", "T2 ", "S0 ", "S1 ", "A0 ", "A1 ", "A2 ", "A3 ", "A4 ", "A5 ",
    "A6 ", "A7 ", "S2 ", "S3 ", "S4 ", "S5 ", "S6 ", "S7 ", "S8 ", "S9 ", "S10", "S11", "T3 ", "T4 ", "T5 ", "T6 ",
};

// Print a register dump given cpu_regs_t.
void kernel_reg_dump_arr(uint32_t const *arr) {
    // Print all registers.
    rawprint("Register dump:\n");
    for (int y = 0; y < 32; y += COLS) {
        for (int x = 0; x < COLS; x++) {
            rawputc(' ');
            rawputc(' ');
            rawprint(regnames[y + x]);
            rawprint(" 0x");
            rawprinthex(arr[y + x], 8);
        }
        rawputc('\n');
    }
}

#define DUMP_CSR(name, id)                                                                                             \
    {                                                                                                                  \
        rawprint(name);                                                                                                \
        long tmp;                                                                                                      \
        asm("csrr %0, " #id : "=r"(tmp));                                                                              \
        rawprinthex(tmp, sizeof(size_t) * 2);                                                                          \
        rawputc('\n');                                                                                                 \
    }

// Print a register dump given isr_ctx_t.
void isr_ctx_dump(isr_ctx_t const *ctx) {
    kernel_reg_dump_arr((uint32_t const *)&ctx->regs);
    DUMP_CSR("  MSTATUS   ", mstatus)
    DUMP_CSR("  MCAUSE    ", mcause)
    DUMP_CSR("  PMPCFG0   ", pmpcfg0)
    DUMP_CSR("  PMPCFG1   ", pmpcfg1)
    DUMP_CSR("  PMPADDR0  ", pmpaddr0)
    DUMP_CSR("  PMPADDR1  ", pmpaddr1)
    DUMP_CSR("  PMPADDR2  ", pmpaddr2)
    DUMP_CSR("  PMPADDR3  ", pmpaddr3)
    DUMP_CSR("  PMPADDR4  ", pmpaddr4)
    DUMP_CSR("  PMPADDR5  ", pmpaddr5)
    DUMP_CSR("  PMPADDR6  ", pmpaddr6)
    DUMP_CSR("  PMPADDR7  ", pmpaddr7)
}



#define isr_noexc_copy_func(width)                                                                                     \
    /* Copy function implementation. */                                                                                \
    static void isr_noexc_copy_u##width##_func(void *cookie) {                                                         \
        uint##width##_t *dest = ((uint##width##_t **)cookie)[0];                                                       \
        uint##width##_t *src  = ((uint##width##_t **)cookie)[1];                                                       \
        *dest                 = *src;                                                                                  \
    }                                                                                                                  \
    /* Try to copy from src to dest. */                                                                                \
    bool isr_noexc_copy_u##width(uint##width##_t *dest, uint##width##_t const *src) {                                  \
        size_t arr[2] = {(size_t)dest, (size_t)src};                                                                   \
        return isr_noexc_run(isr_noexc_copy_u##width##_func, NULL, arr);                                               \
    }

isr_noexc_copy_func(8);
isr_noexc_copy_func(16);
isr_noexc_copy_func(32);
isr_noexc_copy_func(64);
