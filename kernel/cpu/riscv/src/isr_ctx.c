
// SPDX-License-Identifier: MIT

#include "isr_ctx.h"

#include "badge_strings.h"
#include "interrupt.h"
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
void kernel_reg_dump_arr(size_t const *arr) {
    // Print all registers.
    rawprint("Register dump:\n");
    for (int y = 0; y < 32; y += COLS) {
        for (int x = 0; x < COLS; x++) {
            rawputc(' ');
            rawputc(' ');
            rawprint(regnames[y + x]);
            rawprint(" 0x");
            rawprinthex(arr[y + x], sizeof(size_t) * 2);
        }
        rawputc('\n');
    }
}

#define DUMP_CSR(name, id)                                                                                             \
    {                                                                                                                  \
        rawprint(name);                                                                                                \
        long tmp;                                                                                                      \
        asm("csrr %0, " id : "=r"(tmp));                                                                               \
        rawprinthex(tmp, sizeof(size_t) * 2);                                                                          \
        rawputc('\n');                                                                                                 \
    }

// Print a register dump given isr_ctx_t.
void isr_ctx_dump(isr_ctx_t const *ctx) {
    kernel_reg_dump_arr((size_t const *)&ctx->regs);
    rawprint("  HARTID    ");
    rawprinthex(ctx->cpulocal->cpuid, sizeof(size_t) * 2);
    rawputc('\n');
    DUMP_CSR("  STATUS    ", CSR_STATUS_STR)
    DUMP_CSR("  CAUSE     ", CSR_CAUSE_STR)
#if RISCV_M_MODE_KERNEL
    DUMP_CSR("  PMPCFG0   ", "pmpcfg0")
    DUMP_CSR("  PMPCFG1   ", "pmpcfg1")
    DUMP_CSR("  PMPADDR0  ", "pmpaddr0")
    DUMP_CSR("  PMPADDR1  ", "pmpaddr1")
    DUMP_CSR("  PMPADDR2  ", "pmpaddr2")
    DUMP_CSR("  PMPADDR3  ", "pmpaddr3")
    DUMP_CSR("  PMPADDR4  ", "pmpaddr4")
    DUMP_CSR("  PMPADDR5  ", "pmpaddr5")
    DUMP_CSR("  PMPADDR6  ", "pmpaddr6")
    DUMP_CSR("  PMPADDR7  ", "pmpaddr7")
#else
    DUMP_CSR("  SATP      ", "satp")
#endif
}



// Cookie data for `isr_noexc_wrapper`.
typedef struct {
    // Custom trap handler to call, if any.
    isr_catch_t trap_handler;
    // Cookie for trap handler.
    void       *cookie;
    // Regfile to restore in case of trap.
    cpu_regs_t  regfile;
    // Whether a trap was encountered.
    bool        had_trap;
} isr_noexc_cookie_t;

// Trap handler wrapper for `isr_noexc_run`.
static bool isr_noexc_wrapper(isr_ctx_t *kctx, void *cookie0) {
    isr_noexc_cookie_t *cookie = cookie0;
    cookie->had_trap           = true;
    if (cookie->trap_handler) {
        cookie->trap_handler(cookie->cookie, &cookie->regfile);
    }
    mem_copy(&kctx->regs, &cookie->regfile, sizeof(cpu_regs_t));
    return true;
}

// NOLINTNEXTLINE
extern void _isr_noexc_run_int(void *cookie, cpu_regs_t *regs, isr_noexc_t code);

// Run a restricted function and catch exceptions.
// The code will run with interrupts disabled.
// All traps will cause the optional `trap_handler` to be called and `code` to terminate early.
// This should only be used for debug or ISA detection purposes.
// Returns whether a trap occurred.
bool isr_noexc_run(isr_noexc_t code, isr_catch_t trap_handler, void *cookie) {
    isr_noexc_cookie_t data = {
        .trap_handler = trap_handler,
        .cookie       = cookie,
        .had_trap     = false,
    };

    // Set up for custom trap handler.
    bool       ie       = irq_disable();
    isr_ctx_t *kctx     = isr_ctx_get();
    kctx->noexc_cb      = isr_noexc_wrapper;
    kctx->noexc_cookie  = &data;
    kctx->flags        |= ISR_CTX_FLAG_NOEXC;

#if __riscv_xlen == 64
#define ST_REG "sd "
#define LD_REG "ld "
#else
#define ST_REG "sw "
#define LD_REG "lw "
#endif

    // Call the possibly trapping code.
    _isr_noexc_run_int(cookie, &data.regfile, code);

    kctx->flags &= ~ISR_CTX_FLAG_NOEXC;

    irq_enable_if(ie);
    return data.had_trap;
}



// Copy function implementation.
static void isr_noexc_mem_copy_func(void *cookie) {
    void  *dest = ((void **)cookie)[0];
    void  *src  = ((void **)cookie)[1];
    size_t len  = ((size_t *)cookie)[2];
    mem_copy(dest, src, len);
}

// Try to copy memory from src to dest.
// Returns whether an access trap occurred.
bool isr_noexc_mem_copy(void *dest, void const *src, size_t len) {
    size_t arr[3] = {(size_t)dest, (size_t)src, len};
    return isr_noexc_run(isr_noexc_mem_copy_func, NULL, arr);
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
