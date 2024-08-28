
// SPDX-License-Identifier: MIT

#pragma once

#include "port/smp.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



// Number of detected usable CPU cores.
// Never changes after smp_init.
extern int smp_count;

// The the SMP CPU index of the calling CPU.
int    smp_cur_cpu();
// Get the SMP CPU index from the CPU ID value.
int    smp_get_cpu(size_t cpuid);
// Get the CPU ID value from the SMP CPU index.
size_t smp_get_cpuid(int cpu);
// Power on another CPU.
bool   smp_poweron(int cpu, void *entrypoint, void *stack);
// Power off this CPU.
bool   smp_poweroff();
// Pause this CPU, if supported.
bool   smp_pause();
// Resume another CPU, if supported.
bool   smp_resume(int cpu);
// Whether a CPU can be powered off at runtime.
bool   smp_can_poweroff(int cpu);
