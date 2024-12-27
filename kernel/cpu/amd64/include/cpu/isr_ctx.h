
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/msr.h"
#include "cpu/regs.h"

#ifndef __ASSEMBLER__
#include "cpulocal.h"
#include "log.h"
#include "memprotect.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct sched_thread_t sched_thread_t;
typedef struct isr_ctx_t      isr_ctx_t;
// Custom trap handler callback, returns true if the trap was suppressed.
typedef bool (*isr_noexc_cb_t)(isr_ctx_t *ctx, void *cookie);
#endif



#ifdef __ASSEMBLER__

#define STRUCT_BEGIN(structname) .equ structname##_size, 0
#define STRUCT_FIELD(structname, name, len)                                                                            \
    .equ structname##_##name, structname##_size;                                                                       \
    .equ structname##_size, structname##_size + len
#define STRUCT_FIELD_QWORD(structname, name)              STRUCT_FIELD(structname, name, 8)
#define STRUCT_FIELD_DWORD(structname, name)              STRUCT_FIELD(structname, name, 4)
#define STRUCT_FIELD_WORD(structname, name)               STRUCT_FIELD(structname, name, 2)
#define STRUCT_FIELD_PTR(structname, type, name)          STRUCT_FIELD(structname, name, 8)
#define STRUCT_FIELD_STRUCT(structname, type, name, size) STRUCT_FIELD(structname, name, size)
#define STRUCT_END(structname)

#else

#define STRUCT_BEGIN(structname)                                                                                       \
    typedef struct structname structname;                                                                              \
    struct structname {
#define STRUCT_FIELD_QWORD(structname, name)              uint64_t name;
#define STRUCT_FIELD_DWORD(structname, name)              uint32_t name;
#define STRUCT_FIELD_WORD(structname, name)               uint16_t name;
#define STRUCT_FIELD_PTR(structname, type, name)          type *name;
#define STRUCT_FIELD_STRUCT(structname, type, name, size) type name;
#define STRUCT_END(structname)                                                                                         \
    }                                                                                                                  \
    ;

#endif



// Context for interrupts, exceptions and traps in relation to threads.
STRUCT_BEGIN(isr_ctx_t)
// Pointer to currently active memory protection information.
STRUCT_FIELD_PTR(isr_ctx_t, mpu_ctx_t, mpu_ctx)
// Frame pointer to use for backtraces.
STRUCT_FIELD_PTR(isr_ctx_t, void, frameptr)
// Registers storage.
// The trap/interrupt handler will save registers to here.
STRUCT_FIELD_STRUCT(isr_ctx_t, cpu_regs_t, regs, 0)
// Pointer to next isr_ctx_t to switch to.
// If nonnull, the trap/interrupt handler will context switch to this new context before exiting.
STRUCT_FIELD_PTR(isr_ctx_t, isr_ctx_t, ctxswitch)
// Pointer to owning sched_thread_t.
STRUCT_FIELD_PTR(isr_ctx_t, sched_thread_t, thread)
// Kernel context flags, only 32 bits available even on 64-bit targets.
STRUCT_FIELD_QWORD(isr_ctx_t, flags)
// Custom trap handler to call.
STRUCT_FIELD_STRUCT(isr_ctx_t, isr_noexc_cb_t, noexc_cb, 8)
// Cookie for custom trap handler.
STRUCT_FIELD_PTR(isr_ctx_t, void, noexc_cookie)
// Pointer to CPU-local struct.
STRUCT_FIELD_PTR(isr_ctx_t, cpulocal_t, cpulocal)
// Pointer to stack to use for user exceptions.
STRUCT_FIELD_QWORD(isr_ctx_t, user_isr_stack)
STRUCT_END(isr_ctx_t)

// `isr_ctx_t` flag: Is a kernel thread.
#define ISR_CTX_FLAG_KERNEL (1 << 0)
// `isr_ctx_t` flag: Currently running custom trap-handling code.
#define ISR_CTX_FLAG_NOEXC  (1 << 1)
// `isr_ctx_t` flag: Currently in a trap or interrupt handler.
#define ISR_CTX_FLAG_IN_ISR (1 << 2)
// `isr_ctx_t` flag: Currently in a double fault handler (which is fatal).
#define ISR_CTX_FLAG_2FAULT (1 << 3)


#ifndef __ASSEMBLER__

// Stack alignment is defined to be 16 by the SYSV calling convention
#define STACK_ALIGNMENT 16

// Get the current ISR context.
static inline isr_ctx_t *isr_ctx_get() {
    return (isr_ctx_t *)msr_read(MSR_GSBASE);
}
// Get the outstanding context swap target, if any.
static inline isr_ctx_t *isr_ctx_switch_get() {
    isr_ctx_t *rdata;
    asm("mov %0, [gs:%1]" : "=r"(rdata) : "i"((size_t)offsetof(isr_ctx_t, ctxswitch)));
    return rdata;
}
// Set the context swap target to swap to before exiting the trap/interrupt handler.
static inline void isr_ctx_switch_set(isr_ctx_t *switch_to) {
    asm volatile("mov [gs:%1], %0" ::"r"(switch_to), "i"((size_t)offsetof(isr_ctx_t, ctxswitch)) : "memory");
}
// Immediately swap the ISR context handle.
static inline isr_ctx_t *isr_ctx_swap(isr_ctx_t *kctx) {
    isr_ctx_t *old = isr_ctx_get();
    msr_write(MSR_GSBASE, (uint64_t)kctx);
    asm volatile("mov [gs:%1], %0" ::"r"(old->ctxswitch), "i"((size_t)offsetof(isr_ctx_t, ctxswitch)) : "memory");
    asm volatile("mov [gs:%1], %0" ::"r"(old->cpulocal), "i"((size_t)offsetof(isr_ctx_t, cpulocal)) : "memory");
    return NULL;
}
// Print a register dump given isr_ctx_t.
void isr_ctx_dump(isr_ctx_t const *ctx);
// Print a register dump of the current registers.
void kernel_cur_regs_dump();
#endif



#undef STRUCT_BEGIN
#undef STRUCT_FIELD
#undef STRUCT_FIELD_WORD
#undef STRUCT_FIELD_DWORD
#undef STRUCT_FIELD_QWORD
#undef STRUCT_FIELD_PTR
#undef STRUCT_END
