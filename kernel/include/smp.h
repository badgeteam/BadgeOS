
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



// Number of detected usable CPU cores.
// Never changes after smp_init.
extern uint16_t smp_count;

// Initialise the SMP subsystem.
void     smp_init();
// The the SMP CPUID of the calling CPU.
uint16_t smp_cpuid();
// Power on another CPU.
bool     smp_poweron(uint16_t cpu, void *entrypoint, void *stack);
// Power off another CPU.
bool     smp_poweroff(uint16_t cpu);
// Pause another CPU, if supported.
bool     smp_pause(uint16_t cpu);
// Resume another CPU, if supported.
bool     smp_resume(uint16_t cpu);
