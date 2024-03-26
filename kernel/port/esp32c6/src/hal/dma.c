
// SPDX-License-Identifier: MIT

#include "hal/dma.h"

#include "soc/gdma_struct.h"
#include "log.h"
#include "port/clkconfig.h"

void dma_init(badge_err_t *ec, int dma_num) {
    // Bounds check.
    if (dma_num != 0) {
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_RANGE); // TODO: add error location for
        return;
    }

    clkconfig_gmda(true, false);
}

void dma_deinit(badge_err_t *ec, int dma_num) {
    // Bounds check.
    if (dma_num != 0) {
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_RANGE); // TODO: add error location for
        return;
    }

    GDMA.misc_conf.clk_en = 0;
}

void dma_mem_copy(badge_err_t *ec, int dma_num, void *dest, void const *src, size_t size) {
    // Bounds check.
    if (dma_num != 0
     || size > ((1<<12)-1)
    ) {
        badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_RANGE); // TODO: add error location for
        return;
    }

    gdma_descriptor_t rx_desc = {
        .owner = 1,
        .buf = dest,
        .length = size,
        .size = size,
    };
    gdma_descriptor_t tx_desc = {
        .owner = 1,
        .buf = (void *) src,
        .suc_eof = 1,
        .length = size,
        .size = size,
    };

    GDMA.channel[0].out.out_conf0.out_rst = 1;
    GDMA.channel[0].out.out_conf0.out_rst = 0;
    GDMA.channel[0].in.in_conf0.in_rst = 1;
    GDMA.channel[0].in.in_conf0.in_rst = 0;
    GDMA.channel[0].out.out_link.outlink_addr = (uint32_t) & tx_desc;
    GDMA.channel[0].in.in_link.inlink_addr = (uint32_t) & rx_desc;
    GDMA.channel[0].in.in_conf0.mem_trans_en = 1;
    GDMA.channel[0].out.out_link.outlink_start = 1;
    GDMA.channel[0].in.in_link.inlink_start = 1;

    while(GDMA.channel[0].out.out_link.outlink_start);
    while(GDMA.channel[0].in.in_link.inlink_start);
    
}
