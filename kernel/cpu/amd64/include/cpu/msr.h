
// SPDX-License-Identifier: MIT

#pragma once

#ifndef __ASSEMBLER__
#include <stdint.h>
#endif



#ifndef __ASSEMBLER__
// Format of EFER MSR.
typedef union {
    struct {
        // System call extensions.
        uint64_t sce   : 1;
        // Reserved.
        uint64_t       : 7;
        // Long mode enable.
        uint64_t lme   : 1;
        // Long mode active.
        uint64_t lma   : 1;
        // No-execute enable.
        uint64_t nxe   : 1;
        // Secure virtual machine enable.
        uint64_t svme  : 1;
        // Fast FXSAVE/FXSTOR.
        uint64_t ffxsr : 1;
        // Translation cache extension.
        uint64_t tce   : 1;
    };
    uint64_t val;
} msr_efer_t;
#endif



// Address of FSBASE MSR; base address of `fs` segment.
#define MSR_FSBASE  0xc0000100
// Address of GSBASE MSR; base address of `gs` segment.
// Swapped with KGSBASE using the `swapgs` instruction.
#define MSR_GSBASE  0xc0000101
// Address of KGSBASE MSR; temporary value for kernel `gs` segment.
// Swapped with GSBASE using the `swapgs` instruction.
#define MSR_KGSBASE 0xc0000102
// Address of EFER MSR; extended feature enable register.
#define MSR_EFER    0xc0000080



#ifndef __ASSEMBLER__
// Read an MSR.
static inline uint64_t msr_read(uint32_t address) {
    register uint32_t addr asm("ecx") = address;
    register uint32_t lo asm("edx");
    register uint32_t hi asm("eax");
    asm("rdmsr" : "=r"(lo), "=r"(hi) : "r"(addr) : "memory");
    return ((uint64_t)hi << 32) | lo;
}

// Write an MSR.
static inline void msr_write(uint32_t address, uint64_t value) {
    register uint32_t addr asm("ecx") = address;
    register uint32_t lo asm("edx")   = value;
    register uint32_t hi asm("eax")   = value >> 32;
    asm volatile("wrmsr" ::"r"(lo), "r"(hi), "r"(addr) : "memory");
}
#endif
