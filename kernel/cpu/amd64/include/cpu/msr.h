
// SPDX-License-Identifier: MIT

#pragma once



// Address of FSBASE MSR; base address of `fs` segment.
#define MSR_FSBASE  0xc0000100
// Address of GSBASE MSR; base address of `gs` segment.
// Swapped with KGSBASE using the `swapgs` instruction.
#define MSR_GSBASE  0xc0000101
// Address of KGSBASE MSR; temporary value for kernel `gs` segment.
// Swapped with GSBASE using the `swapgs` instruction.
#define MSR_KGSBASE 0xc0000102
