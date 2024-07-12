
// SPDX-License-Identifier: MIT

#include "smp.h"



// CPU1 stack pointer.
void *cpu1_temp_stack;

// Number of detected CPU cores.
uint16_t smp_count = 1;

// Initialise the SMP subsystem.
void smp_init() {
}

// The the SMP CPUID of the calling CPU.
uint16_t smp_cpuid() {
    return 0;
}

// Power on another CPU.
bool smp_poweron(uint16_t cpu, void *entrypoint, void *stack) {
    return false;
}

// Power off another CPU.
bool smp_poweroff(uint16_t cpu) {
    return false;
}

// Pause another CPU, if supported.
bool smp_pause(uint16_t cpu) {
    return false;
}

// Resume another CPU, if supported.
bool smp_resume(uint16_t cpu) {
    return false;
}
