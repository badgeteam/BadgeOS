#include "scheduler.h"

#include "assertions.h"
#include "kernel_ctx.h"
#include "log.h"
#include "memory.h"

// The trampoline is used to jump into the thread code and return from it,
// ensuring that we can detect when a thread has exited.
static void thread_trampoline(sched_entry_point_t ep, void *arg) ALIGNED_TO(4);
static void thread_trampoline(sched_entry_point_t ep, void *arg) {
    logk(LOG_INFO, "starting thread...");
    assert_always(ep != NULL);

    sched_thread_t *const this_thread = sched_get_current_thread();
    assert_always(this_thread != NULL);

    ep(arg);

    // make sure the thread is always exited properly
    logk(LOG_INFO, "thread done!");
    sched_exit(0);
}

void sched_prepare_kernel_entry(
    kernel_ctx_t *const       ctx,
    uintptr_t const           initial_stack_pointer,
    sched_entry_point_t const entry_point,
    void *const               arg
) {
    mem_set(&ctx->regs, 0, sizeof(cpu_regs_t));

    // setup the trampoline
    ctx->regs.pc = (uintptr_t)thread_trampoline;
    ctx->regs.sp = initial_stack_pointer;
    ctx->regs.a0 = (uintptr_t)entry_point;
    ctx->regs.a1 = (uintptr_t)arg;

    // Copy over TP and GP from the current context
    asm volatile("" : "={gp}"(ctx->regs.gp));
    asm volatile("" : "={tp}"(ctx->regs.tp));
}


void sched_prepare_user_entry(kernel_ctx_t *const ctx, sched_entry_point_t const entry_point, void *const arg) {

    ctx->regs.pc = (uintptr_t)entry_point;
    ctx->regs.a0 = (uintptr_t)arg;

    // return to invalid code so we get a crash
    ctx->regs.ra = 0xDEADC0DEUL;

    // Setup the rest of the registers to bogus values
    // to early-on detect
    ctx->regs.sp = 0xDEADC0DEUL;
    ctx->regs.gp = 0xDEADC0DEUL;
    ctx->regs.tp = 0xDEADC0DEUL;
}