
// SPDX-License-Identifier: MIT

#pragma once

#ifndef __ASSEMBLER__
#include <stdint.h>
#endif



// Address of FSBASE MSR; base address of `fs` segment.
#define MSR_FSBASE  0xc0000100
// Address of GSBASE MSR; base address of `gs` segment.
// Swapped with KGSBASE using the `swapgs` instruction.
#define MSR_GSBASE  0xc0000101
// Address of KGSBASE MSR; temporary value for kernel `gs` segment.
// Swapped with GSBASE using the `swapgs` instruction.
#define MSR_KGSBASE 0xc0000102



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
