
// SPDX-License-Identifier: MIT

#include "smp.h"

#include "cpu/panic.h"
#include "hal/cpu_utility_ll.h"
#include "isr_ctx.h"
#include "rom/ets_sys.h"
#include "soc/hp_sys_clkrst_struct.h"



// CPU1 entrypoint.
static void (*cpu1_entry)();
// CPU1 stack pointer.
static void      *cpu1_temp_stack;
// CPU1 local data.
extern cpulocal_t port_cpu1_local;

// APP CPU startup stub.
static void appcpu_stub() __attribute__((naked));
static void appcpu_stub() {
    asm(".option push\n"
        ".option norelax\n"
        "la gp, __global_pointer$\n"
        ".option pop\n"
        "lw sp, cpu1_temp_stack\n"
        "j appcpu_stub_2");
}

// APP CPU startup stub: Part 2 edition.
static void appcpu_stub_2() __attribute__((unused));
static void appcpu_stub_2() {
    isr_ctx_t kctx  = {0};
    kctx.flags     |= ISR_CTX_FLAG_KERNEL | ISR_CTX_FLAG_USE_SP;
    kctx.cpulocal   = &port_cpu1_local;
    asm("csrw mscratch, %0" ::"r"(&kctx));
    cpu1_entry();
    logk_from_isr(LOG_FATAL, "CPU1 entry function returned");
    panic_abort();
}

// Number of detected CPU cores.
int smp_count = 2;

// The the SMP CPUID of the calling CPU.
int smp_cur_cpu() {
    int mhartid;
    asm("csrr %0, mhartid" : "=r"(mhartid));
    return mhartid;
}

// Get the SMP CPU index from the CPU ID value.
int smp_get_cpu(size_t cpuid) {
    return (int)cpuid;
}

// Get the CPU ID value from the SMP CPU index.
size_t smp_get_cpuid(int cpu) {
    return (size_t)cpu;
}

// Power on another CPU.
bool smp_poweron(int cpu, void *entrypoint, void *stack) {
    if (cpu != 1) {
        return false;
    }
    cpu_utility_ll_stall_cpu(1);
    HP_SYS_CLKRST.soc_clk_ctrl0.reg_core1_cpu_clk_en = true;
    HP_SYS_CLKRST.hp_rst_en0.reg_rst_en_core1_global = false;
    cpu_utility_ll_reset_cpu(1);
    cpu1_entry      = entrypoint;
    cpu1_temp_stack = stack;
    ets_set_appcpu_boot_addr((size_t)appcpu_stub);
    asm("fence w,w");
    cpu_utility_ll_unstall_cpu(1);
    return true;
}

// Power off this CPU.
bool smp_poweroff() {
    int cpu = smp_cur_cpu();
    if (cpu == 0) {
        return false;
    } else if (cpu == 1) {
        HP_SYS_CLKRST.soc_clk_ctrl0.reg_core1_cpu_clk_en = false;
    }
    return true;
}


// Pause this CPU, if supported.
bool smp_pause() {
    cpu_utility_ll_stall_cpu(smp_cur_cpu());
    return true;
}

// Resume another CPU, if supported.
bool smp_resume(int cpu) {
    cpu_utility_ll_unstall_cpu(cpu);
    return true;
}

// Whether a CPU can be powered off at runtime.
bool smp_can_poweroff(int cpu) {
    return cpu == 1;
}
