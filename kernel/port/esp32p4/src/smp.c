
// SPDX-License-Identifier: MIT

#include "smp.h"

#include "hal/cpu_utility_ll.h"
#include "rom/ets_sys.h"
#include "soc/hp_sys_clkrst_struct.h"



// CPU1 stack pointer.
void *cpu1_temp_stack;

// Number of detected CPU cores.
uint16_t smp_count = 2;

// Initialise the SMP subsystem.
void smp_init() {
}

// The the SMP CPUID of the calling CPU.
uint16_t smp_cpuid() {
    size_t mhartid;
    asm("csrr %0, mhartid" : "=r"(mhartid));
    return mhartid;
}

// Power on another CPU.
bool smp_poweron(uint16_t cpu, void *entrypoint, void *stack) {
    cpu_utility_ll_stall_cpu(1);
    HP_SYS_CLKRST.soc_clk_ctrl0.reg_core1_cpu_clk_en = true;
    HP_SYS_CLKRST.hp_rst_en0.reg_rst_en_core1_global = false;
    cpu_utility_ll_reset_cpu(1);
    ets_set_appcpu_boot_addr((uint32_t)entrypoint);
    cpu1_temp_stack = stack;
    cpu_utility_ll_unstall_cpu(1);
    return true;
}

// Power off another CPU.
bool smp_poweroff(uint16_t cpu) {
    if (cpu == 0) {
        return false;
    }
    HP_SYS_CLKRST.soc_clk_ctrl0.reg_core1_cpu_clk_en = false;
    HP_SYS_CLKRST.hp_rst_en0.reg_rst_en_core1_global = true;
    return true;
}

// Pause another CPU, if supported.
bool smp_pause(uint16_t cpu) {
    cpu_utility_ll_stall_cpu(cpu);
    return true;
}

// Resume another CPU, if supported.
bool smp_resume(uint16_t cpu) {
    cpu_utility_ll_unstall_cpu(cpu);
    return true;
}
