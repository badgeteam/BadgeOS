
// SPDX-License-Identifier: MIT

#pragma once

#include "badge_err.h"

#include <stdint.h>

// Returns the amount of DMA peripherals present.
// Cannot produce an error.
#define dma_count() (1)

typedef union gdma_descriptor_u {
    struct {
        /** size : RW; bitpos: [11:0]; default: 0;
         *  This field stores the size of the buffer that this descriptor points to.
         */
        uint32_t size:12;
        /** size : RW; bitpos: [23:12]; default: 0;
         *  This field stores the number of valid bytes in the buffer that this descriptor points to.
         */
        uint32_t length:12;
        uint32_t reserved_24:4;
        /** err_eof : RO; bitpos: [28]; default: 0;
         *  Specifies whether the received data has errors.
         */
        uint32_t err_eof:1;
        uint32_t reserved_29:1;
        /** suc_eof : RO; bitpos: [30]; default: 0;
         *  Specifies whether this descriptor is the last descriptor of a data frame or packet.
         */
        uint32_t suc_eof:1;
        /** owner : RW; bitpos: [31]; default: 0;
         *  Specifies who is allowed to access the buffer that this descriptor points to.
         *  0: CPU can access the buffer.
         *  1: The GDMA controller can access the buffer.
         */
        uint32_t owner:1;
        void *buf;
        union gdma_descriptor_u *next;
    };
    uint32_t val[3];
} gdma_descriptor_t;

void dma_init(badge_err_t *ec, int dma_num);
void dma_deinit(badge_err_t *ec, int dma_num);
void dma_mem_copy(badge_err_t *ec, int dma_num, void *dest, void const *src, size_t size);
