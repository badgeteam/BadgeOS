
// SPDX-License-Identifier: MIT

#include "isr_ctx.h"

#include "badge_strings.h"
#include "interrupt.h"
#include "rawprint.h"

// Number of columns in register dump.
// Must be a power of 2.
#define COLS 4

// Register names table.
static char const regnames[16][4] = {
    // clang-format off
    "RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "RSP", "RBP",
    "R8 ", "R9 ", "R10", "R11", "R12", "R13", "R14", "R15",
    // clang-format on
};

// Print a register dump given cpu_regs_t.
void kernel_reg_dump_arr(size_t const *arr) {
    // Print all registers.
    rawprint("Register dump:\n");
    for (int y = 0; y < 16; y += COLS) {
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

#define DUMP_SEG(name, id)                                                                                             \
    {                                                                                                                  \
        rawprint(name);                                                                                                \
        rawprinthex(ctx->regs.id, sizeof(size_t) * 2);                                                                 \
        rawputc('\n');                                                                                                 \
    }

// Print a register dump given isr_ctx_t.
void isr_ctx_dump(isr_ctx_t const *ctx) {
    kernel_reg_dump_arr((size_t const *)&ctx->regs);
    DUMP_SEG("  CS  ", cs);
    DUMP_SEG("  DS  ", ds);
    DUMP_SEG("  SS  ", ss);
    DUMP_SEG("  ES  ", es);
    DUMP_SEG("  FS  ", fs);
    DUMP_SEG("  GS  ", gs);
    rawprint("  CPUID   ");
    rawprinthex(ctx->cpulocal->cpuid, sizeof(size_t) * 2);
    rawputc('\n');
    rawprint("  RFLAGS  ");
    rawprinthex(ctx->cpulocal->cpuid, sizeof(size_t) * 2);
    rawputc('\n');
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
