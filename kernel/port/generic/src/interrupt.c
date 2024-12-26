
// SPDX-License-Identifier: MIT

#include "interrupt.h"

#include "cpu/isr.h"

#ifdef __x86_64__
#include "cpu/msr.h"
#include "cpu/priv_level.h"
#include "cpu/segmentation.h"



// Global descriptor table.
static uint64_t gdt[] = {
    0,
    // Entry 1 is for kernel code.
    GDT_FLAG_L | GDT_ACCESS_E | GDT_ACCESS_RW | GDT_ACCESS_P,
    // Entry 2 is for kernel data.
    GDT_FLAG_L | GDT_ACCESS_RW | GDT_ACCESS_P,
    // Entry 3 is for user code.
    GDT_FLAG_L | GDT_ACCESS_E | GDT_ACCESS_RW | GDT_ACCESS_P | (PRIV_USER << GDT_ACCESS_DPL_BASE),
    // Entry 4 is for user data.
    GDT_FLAG_L | GDT_ACCESS_RW | GDT_ACCESS_P | (PRIV_USER << GDT_ACCESS_DPL_BASE),
};

// Set up the GDT in BadgerOS-owned memory.
void x86_setup_gdt() {
    struct PACKED {
        uint16_t size;
        void    *addr;
    } gdtr = {
        4,
        gdt,
    };
    asm volatile("lgdt [%0]" ::"m"(gdtr));
}

// Reload the segment registers.
void x86_reload_segments() NAKED;
void x86_reload_segments() {
    // Load the global descriptor table and update segment registers.
    // clang-format off
    asm volatile(
        "pushq %0;"
        "lea rax, [1f];"
        "push rax;"
        "retfq;"
        "1:;"
        "mov ax, %1;"
        "mov ds, ax;"
        "mov es, ax;"
        "mov fs, ax;"
        "mov gs, ax;"
        "mov ss, ax;"
        "ret;"
        ::
        "i"(FORMAT_SEGMENT(SEGNO_KCODE, 0, PRIV_KERNEL)),
        "i"(FORMAT_SEGMENT(SEGNO_KDATA, 0, PRIV_KERNEL))
        :
        "rax", "memory"
    );
    // clang-format on
}
#endif



// Temporary interrupt context before scheduler.
static isr_ctx_t tmp_ctx = {.flags = ISR_CTX_FLAG_KERNEL};

// Initialise interrupt drivers for this CPU.
void irq_init() {
    // TODO: Proper way to do this.
#ifdef __x86_64__
    // Set up GDT for booting CPU.
    x86_setup_gdt();
    x86_reload_segments();
    // Set GSBASE to the address of the ISR context.
#else
    // Install interrupt handler.
    asm volatile("csrw sstatus, 0");
    asm volatile("csrw stvec, %0" ::"r"(riscv_interrupt_vector_table));
    asm volatile("csrw sscratch, %0" ::"r"(&tmp_ctx));

    // Disable all internal interrupts.
    asm volatile("csrw sie, 0");
#endif
}
