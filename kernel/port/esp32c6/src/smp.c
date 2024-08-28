
// SPDX-License-Identifier: MIT

#include "smp.h"



// Number of detected usable CPU cores.
// Never changes after smp_init.
int smp_count = 1;

// The the SMP CPU index of the calling CPU.
int smp_cur_cpu() {
    return 0;
}

// Get the SMP CPU index from the CPU ID value.
int smp_get_cpu(size_t cpuid) {
    (void)cpuid;
    return 0;
}

// Get the CPU ID value from the SMP CPU index.
size_t smp_get_cpuid(int cpu) {
    (void)cpu;
    return 0;
}

// Power on another CPU.
bool smp_poweron(int cpu, void *entrypoint, void *stack) {
    (void)cpu;
    (void)entrypoint;
    (void)stack;
    return false;
}

// Power off this CPU.
bool smp_poweroff() {
    return false;
}

// Pause this CPU, if supported.
bool smp_pause() {
    return false;
}

// Resume another CPU, if supported.
bool smp_resume(int cpu) {
    (void)cpu;
    return false;
}

// Whether a CPU can be powered off at runtime.
bool smp_can_poweroff(int cpu) {
    (void)cpu;
    return false;
}
