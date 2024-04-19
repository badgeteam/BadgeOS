
// SPDX-License-Identifier: MIT

#include "port/pmu_init.h"

#include "badge_strings.h"
#include "soc/pmu_struct.h"
#include "soc/soc_types.h"



// TODO: More proper method than copying ESP-IDF's values.
static uint32_t const pmu_rom[] = {
    0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x7f800000, 0x02ec0000, 0x010000a0, 0x07801bc0,
    0x08000000, 0xc0007180, 0x00000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00200000,
    0x00000000, 0x00000000, 0x00000000, 0x31000000, 0x00e00000, 0xc0000000, 0x12800200, 0x07801bc0, 0x30000000,
    0xc0040000, 0x00000000, 0x00000000, 0xe8400000, 0x00000000, 0x00000000, 0x00000000, 0x40000000, 0x00000000,
    0xc0400000, 0x00000000, 0x00000000, 0x40000000, 0x00000000, 0xc0000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x7fbfdfe0, 0x7fbfdfe0, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000100,
    0x00000000, 0x00000000, 0x00000000, 0x00020000, 0x00000000, 0x01000080, 0x00000080, 0x00010000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000032, 0x00000a0a, 0x80000000, 0x09000000, 0x80000000, 0x00028000,
    0x00000000, 0x00000000, 0x00000000, 0x00028000, 0x00000000, 0x00000000, 0x00000000, 0x1ff00001, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x08100801, 0x00802000, 0xf8f8407f,
    0xffffffff, 0xffffffff, 0x40200180, 0xf3800000, 0x40200000, 0xa0000000, 0x40200000, 0xa0000000, 0x40200180,
    0x92000000, 0x40200000, 0xa0000000, 0x40200000, 0xa0000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x000003ff, 0x000f0000, 0x00000000, 0x00100000, 0x004b0205, 0x00000000, 0x00190140, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x02303140, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000,
    0x7f800000, 0x02ec0000, 0x010000a0, 0x07801bc0, 0x08000000, 0xc0007180, 0x00000000, 0x80000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00200000, 0x00000000, 0x00000000, 0x00000000, 0x31000000, 0x00e00000,
    0xc0000000, 0x12800200, 0x07801bc0, 0x30000000, 0xc0040000, 0x00000000, 0x00000000, 0xe8400000, 0x00000000,
    0x00000000, 0x00000000, 0x40000000, 0x00000000, 0xc0400000, 0x00000000, 0x00000000, 0x40000000, 0x00000000,
    0xc0000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x7fbfdfe0, 0x7fbfdfe0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x01000100, 0x00000000, 0x00000000, 0x00000000, 0x00020000, 0x00000000,
    0x01000080, 0x00000080, 0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000032, 0x00000a0a,
    0x80000000, 0x09000000, 0x80000000, 0x00028000, 0x00000000, 0x00000000, 0x00000000, 0x00028000, 0x00000000,
    0x00000000, 0x00000000, 0x1ff00001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x08100801, 0x00802000, 0xf8f8407f, 0xffffffff, 0xffffffff, 0x40200180, 0xf3800000, 0x40200000,
    0xa0000000, 0x40200000, 0xa0000000, 0x40200180, 0x92000000, 0x40200000, 0xa0000000, 0x40200000, 0xa0000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000003ff, 0x000f0000, 0x00000000, 0x00100000, 0x004b0205,
    0x00000000, 0x00190140, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x02303140, 0x00000000,
    0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x7f800000, 0x02ec0000, 0x010000a0, 0x07801bc0, 0x08000000,
    0xc0007180, 0x00000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00200000, 0x00000000,
    0x00000000, 0x00000000, 0x31000000, 0x00e00000, 0xc0000000, 0x12800200, 0x07801bc0, 0x30000000, 0xc0040000,
    0x00000000, 0x00000000, 0xe8400000, 0x00000000, 0x00000000, 0x00000000, 0x40000000, 0x00000000, 0xc0400000,
    0x00000000, 0x00000000, 0x40000000, 0x00000000, 0xc0000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x7fbfdfe0, 0x7fbfdfe0, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000100, 0x00000000,
    0x00000000, 0x00000000, 0x00020000, 0x00000000, 0x01000080, 0x00000080, 0x00010000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000032, 0x00000a0a, 0x80000000, 0x09000000, 0x80000000, 0x00028000, 0x00000000,
    0x00000000, 0x00000000, 0x00028000, 0x00000000, 0x00000000, 0x00000000, 0x1ff00001, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x08100801, 0x00802000, 0xf8f8407f, 0xffffffff,
    0xffffffff, 0x40200180, 0xf3800000, 0x40200000, 0xa0000000, 0x40200000, 0xa0000000, 0x40200180, 0x92000000,
    0x40200000, 0xa0000000, 0x40200000, 0xa0000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000003ff,
    0x000f0000, 0x00000000, 0x00100000, 0x004b0205, 0x00000000, 0x00190140, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x02303140, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x7f800000,
    0x02ec0000, 0x010000a0, 0x07801bc0, 0x08000000, 0xc0007180, 0x00000000, 0x80000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00200000, 0x00000000, 0x00000000, 0x00000000, 0x31000000, 0x00e00000, 0xc0000000,
    0x12800200, 0x07801bc0, 0x30000000, 0xc0040000, 0x00000000, 0x00000000, 0xe8400000, 0x00000000, 0x00000000,
    0x00000000, 0x40000000, 0x00000000, 0xc0400000, 0x00000000, 0x00000000, 0x40000000, 0x00000000, 0xc0000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x7fbfdfe0,
    0x7fbfdfe0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x01000100, 0x00000000, 0x00000000, 0x00000000, 0x00020000, 0x00000000, 0x01000080,
    0x00000080, 0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000032, 0x00000a0a, 0x80000000,
    0x09000000, 0x80000000, 0x00028000, 0x00000000, 0x00000000, 0x00000000, 0x00028000, 0x00000000, 0x00000000,
    0x00000000, 0x1ff00001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x08100801, 0x00802000, 0xf8f8407f, 0xffffffff, 0xffffffff, 0x40200180, 0xf3800000, 0x40200000, 0xa0000000,
    0x40200000, 0xa0000000, 0x40200180, 0x92000000, 0x40200000, 0xa0000000, 0x40200000, 0xa0000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x000003ff, 0x000f0000, 0x00000000, 0x00100000, 0x004b0205, 0x00000000,
    0x00190140, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x02303140,
};

// Initialise the power management unit.
void pmu_init() {
    mem_copy(&PMU, pmu_rom, sizeof(pmu_rom));
}
