
// SPDX-License-Identifier: MIT

#include "cpu/riscv_plic.h"

#include "assertions.h"
#include "driver.h"
#include "interrupt.h"
#include "malloc.h"
#include "smp.h"



// PLIC context descriptor.
typedef struct {
    // HART ID of target CPU.
    size_t  hartid;
    // Target CPU interrupt number.
    uint8_t irq;
} plic_ctx_t;

// PLIC base address.
static size_t      plic_base;
// Number of PLIC contexts.
static uint16_t    plic_ctx_count;
// PLIC contexts.
static plic_ctx_t *plic_ctx;
// PLIC context to use per SMP CPU.
static uint16_t   *plic_smp_ctx;


// Enable an interrupt for a specific CPU.
void irq_ch_enable_affine(int irq, int cpu_index) {
}

// Disable an interrupt for a specific CPU.
void irq_ch_disable_affine(int irq, int cpu_index) {
}

// Enable the IRQ.
void irq_ch_enable(int irq) {
}

// Disable the IRQ.
void irq_ch_disable(int irq) {
}

// Query whether the IRQ is enabled.
bool irq_ch_is_enabled(int irq) {
    return false;
}



// Init PLIC driver from DTB.
void plic_dtbinit(dtb_handle_t *dtb, dtb_entity_t node, uint32_t addr_cells, uint32_t size_cells) {
    // Read PLIC properties.
    plic_base = dtb_read_cells(dtb, node, "reg", 0, addr_cells);
    assert_always(dtb_read_uint(dtb, node, "#address-cells") == 0);
    assert_always(dtb_read_uint(dtb, node, "#interrupt-cells") == 1);

    // Read interrupt mappings.
    dtb_entity_t int_ext = dtb_get_prop(dtb, node, "interrupts-extended");
    plic_ctx_count       = int_ext.prop_len / 8;
    plic_ctx             = malloc(plic_ctx_count * sizeof(plic_ctx_t));
    plic_smp_ctx         = malloc(sizeof(uint16_t) * smp_count);

    // Read interrupt context mappings.
    for (uint16_t i = 0; i < plic_ctx_count; i++) {
        uint32_t     phandle = dtb_prop_read_cell(dtb, int_ext, i * 2);
        dtb_entity_t ictl    = dtb_phandle_node(dtb, phandle);
        if (!ictl.valid) {
            logkf(LOG_ERROR, "Unable to find interrupt controller %{u32;d}", phandle);
            continue;
        }
        plic_ctx[i].irq   = dtb_prop_read_cell(dtb, int_ext, i * 2 + 1);
        dtb_entity_t cpu  = dtb_find_parent(dtb, ictl);
        dtb_entity_t cpus = dtb_find_parent(dtb, cpu);
        if (!cpu.valid) {
            logkf(LOG_ERROR, "Unable to find CPU for interrupt controller %{u32;d}", phandle);
        } else {
            uint32_t cpu_acell = dtb_read_uint(dtb, cpus, "#address-cells");
            size_t   cpu_id    = dtb_read_cells(dtb, cpu, "reg", 0, cpu_acell);
            logkf(LOG_INFO, "CPU for interrupt controller %{u32;d} ID is 0x%{size;x}", phandle, cpu_id);
        }
    }
}

// Define PLIC driver.
DRIVER_DECL(riscv_plic_driver) = {
    .dtb_supports_len = 2,
    .dtb_supports     = (char const *[]){"sifive,plic-1.0.0", "riscv,plic0"},
    .dtbinit          = plic_dtbinit,
};
