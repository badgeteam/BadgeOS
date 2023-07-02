
// SPDX-License-Identifier: MIT

#pragma once

#ifndef __ASSEMBLER__
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#endif


#ifdef __ASSEMBLER__

#define STRUCT_BEGIN(structname)
#define STRUCT_FIELD_WORD(structname, name, offset) \
	.equ structname ## _ ## name, offset
#define STRUCT_FIELD_PTR(structname, type, name, offset) \
	.equ structname ## _ ## name, offset
#define STRUCT_END(structname)

#else

#define STRUCT_BEGIN(structname) \
	typedef struct {
#define STRUCT_FIELD_WORD(structname, name, offset) \
	uint32_t name;
#define STRUCT_FIELD_PTR(structname, type, name, offset) \
	type *name;
#define STRUCT_END(structname) \
	} structname;

#endif


// RISC-V register file copy.
STRUCT_BEGIN(cpu_regs_t)
STRUCT_FIELD_WORD(cpu_regs_t, pc, 0)
STRUCT_FIELD_WORD(cpu_regs_t, ra, 4)
STRUCT_FIELD_WORD(cpu_regs_t, sp, 8)
STRUCT_FIELD_WORD(cpu_regs_t, gp, 12)
STRUCT_FIELD_WORD(cpu_regs_t, tp, 16)
STRUCT_FIELD_WORD(cpu_regs_t, t0, 20)
STRUCT_FIELD_WORD(cpu_regs_t, t1, 24)
STRUCT_FIELD_WORD(cpu_regs_t, t2, 28)
STRUCT_FIELD_WORD(cpu_regs_t, s0, 32)
STRUCT_FIELD_WORD(cpu_regs_t, s1, 36)
STRUCT_FIELD_WORD(cpu_regs_t, a0, 40)
STRUCT_FIELD_WORD(cpu_regs_t, a1, 44)
STRUCT_FIELD_WORD(cpu_regs_t, a2, 48)
STRUCT_FIELD_WORD(cpu_regs_t, a3, 52)
STRUCT_FIELD_WORD(cpu_regs_t, a4, 56)
STRUCT_FIELD_WORD(cpu_regs_t, a5, 60)
STRUCT_FIELD_WORD(cpu_regs_t, a6, 64)
STRUCT_FIELD_WORD(cpu_regs_t, a7, 68)
STRUCT_FIELD_WORD(cpu_regs_t, s2, 72)
STRUCT_FIELD_WORD(cpu_regs_t, s3, 76)
STRUCT_FIELD_WORD(cpu_regs_t, s4, 80)
STRUCT_FIELD_WORD(cpu_regs_t, s5, 84)
STRUCT_FIELD_WORD(cpu_regs_t, s6, 88)
STRUCT_FIELD_WORD(cpu_regs_t, s7, 92)
STRUCT_FIELD_WORD(cpu_regs_t, s8, 96)
STRUCT_FIELD_WORD(cpu_regs_t, s9, 100)
STRUCT_FIELD_WORD(cpu_regs_t, s10, 104)
STRUCT_FIELD_WORD(cpu_regs_t, s11, 108)
STRUCT_FIELD_WORD(cpu_regs_t, t3, 112)
STRUCT_FIELD_WORD(cpu_regs_t, t4, 116)
STRUCT_FIELD_WORD(cpu_regs_t, t5, 120)
STRUCT_FIELD_WORD(cpu_regs_t, t6, 124)
STRUCT_END(cpu_regs_t)


#ifndef __ASSEMBLER__
typedef cpu_regs_t cpu_regs_t;
#endif


#undef STRUCT_BEGIN
#undef STRUCT_FIELD_WORD
#undef STRUCT_FIELD_PTR
#undef STRUCT_END
