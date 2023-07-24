
// SPDX-License-Identifier: MIT

#include "port/intmtx.h"
#include "port/hardware.h"



static uint16_t offsets[] = {
    0x0034, 0x0038, 0x003C, 0x0040, 0x0044, 0x0048, 0x004C, 0x0050, 0x0054, 0x0058, 0x005C, 0x0060, 0x0064,
    0x0068, 0x006C, 0x0070, 0x0074, 0x0078, 0x007C, 0x0080, 0x0084, 0x008C, 0x0090, 0x0094, 0x0098, 0x009C,
    0x00A0, 0x00A4, 0x00A8, 0x00AC, 0x00B0, 0x00B4, 0x00B8, 0x00BC, 0x00C0, 0x00C4, 0x00C8, 0x00CC, 0x00D0,
    0x00D4, 0x00D8, 0x00DC, 0x00E0, 0x00E4, 0x00E8, 0x00EC, 0x00F0, 0x00F4, 0x00F8, 0x00FC, 0x0100, 0x0104,
    0x0108, 0x010C, 0x0110, 0x0114, 0x0118, 0x011C, 0x0120, 0x0124, 0x0128, 0x012C, 0x0130,
};



// Initialise the interrupt matrix with defaults.
void intmtx_init() {
    // Unroute all interrupt channels.
    for (size_t i = 0; i < sizeof(offsets) / sizeof(uint16_t); i++) {
        WRITE_REG(INTMTX_BASE + offsets[i], 0);
    }
    
    // Reset PLIC.
    for (size_t i = 0; i < 32; i++) {
        WRITE_REG(PLIC_MX_INT_PRI_N_REG(i), 0);
    }
    WRITE_REG(PLIC_MX_INT_TYPE_REG, 0);
    WRITE_REG(PLIC_MX_INT_ENABLE_REG, 0);
    WRITE_REG(PLIC_MX_INT_THRESH_REG, 0);
    WRITE_REG(PLIC_MX_INT_CLEAR_REG, 0xffffffff);
    WRITE_REG(PLIC_MX_INT_CLEAR_REG, 0);
    
    // Reset INTPRI.
    for (size_t i = 0; i < 32; i++) {
        WRITE_REG(INTPRI_CORE0_CPU_INT_PRI_N_REG(i), 0);
    }
    WRITE_REG(INTPRI_CORE0_CPU_INT_TYPE_REG, 0);
    WRITE_REG(INTPRI_CORE0_CPU_INT_ENABLE_REG, 0);
    WRITE_REG(INTPRI_CORE0_CPU_INT_THRESH_REG, 0);
    WRITE_REG(INTPRI_CORE0_CPU_INT_CLEAR_REG, 0xffffffff);
    WRITE_REG(INTPRI_CORE0_CPU_INT_CLEAR_REG, 0);
}

// Route interrupt source `source_map_reg` to interrupt channel `channel`.
void intmtx_route(int source_map_reg, int channel) {
    WRITE_REG(source_map_reg, channel);
}

// Set interrupt channel `channel` priority to `prio`.
void intmtx_set_prio(int channel, int prio) {
    WRITE_REG(PLIC_MX_INT_PRI_N_REG(channel), prio);
}

// Set interrupt channel `channel` priority to `prio`.
int intmtx_get_prio(int channel) {
    return READ_REG(PLIC_MX_INT_PRI_N_REG(channel));
}

// Set interrupt threshold value.
void intmtx_set_thresh(int thresh) {
    WRITE_REG(PLIC_MX_INT_THRESH_REG, thresh);
}

// Query whether interrupt channel `channel` is enabled.
bool intmtx_is_enabled(int channel) {
    return READ_REG(PLIC_MX_INT_ENABLE_REG) & (1 << channel);
}

// Enable or disable interrupt channel `channel`.
void intmtx_enable(int channel) {
    WRITE_REG(PLIC_MX_INT_ENABLE_REG, READ_REG(PLIC_MX_INT_ENABLE_REG) | (1 << channel));
}

// Enable or disable interrupt channel `channel`.
bool intmtx_disable(int channel) {
    uint32_t orig = READ_REG(PLIC_MX_INT_ENABLE_REG);
    WRITE_REG(PLIC_MX_INT_ENABLE_REG, orig & ~(1 << channel));
    return orig & (1 << channel);
}

// Acknowledge an interrupt.
void intmtx_ack(int channel) {
    WRITE_REG(INTPRI_CORE0_CPU_INT_CLEAR_REG, 1 << channel);
    WRITE_REG(INTPRI_CORE0_CPU_INT_CLEAR_REG, 0);
}
