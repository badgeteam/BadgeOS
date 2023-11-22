
// SPDX-License-Identifier: MIT

#include "syscall.h"

#include "cpu/isr.h"
#include "cpu/isr_ctx.h"
#include "cpu/panic.h"
#include "rawprint.h"
#include "scheduler/cpu.h"
#include "scheduler/scheduler.h"



// Called on invalid system call.
static void invalid_syscall(long sysno) {
    isr_ctx_t *ctx = isr_ctx_get();

    // Report error.
    rawprint("Invalid syscall ");
    rawprintdec(sysno, 0);
    rawprint(" at PC 0x");
    rawprinthex(ctx->regs.pc, sizeof(ctx->regs.pc) * 2);
    rawprint("\n");
    isr_ctx_dump(ctx);

    // TODO: Terminate thread.
    panic_poweroff();
}

// System call handler jump table thing.
__SYSCALL_HANDLER_SIGNATURE {
    __SYSCALL_HANDLER_IGNORE_UNUSED;

    long long retval = 0;
    switch (sysnum) {
        case SYSCALL_TEMP_WRITE: rawprint_substr((char const *)a0, (size_t)a1); break;
        case SYSCALL_THREAD_YIELD: sched_yield(); break;
        case SYSCALL_THREAD_CREATE: /* TODO */ break;
        case SYSCALL_THREAD_SUSPEND: /* TODO */ break;
        case SYSCALL_THREAD_RESUME: /* TODO */ break;
        case SYSCALL_THREAD_DETACH: /* TODO */ break;
        case SYSCALL_THREAD_DESTROY: /* TODO */ break;
        case SYSCALL_THREAD_EXIT: /* TODO */ break;
        default: invalid_syscall(sysnum); break;
    }
    __syscall_return(retval);
}
