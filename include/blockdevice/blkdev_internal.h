
// SPDX-License-Identifier: MIT

#pragma once

#include "blockdevice.h"

// Write without caching.
void blkdev_write_raw(badge_err_t *ec, blkdev_t *dev, blksize_t block, uint8_t const *writebuf);
// Read without caching.
void blkdev_read_raw(badge_err_t *ec, blkdev_t *dev, blksize_t block, uint8_t *readbuf);
// Erase without caching.
void blkdev_erase_raw(badge_err_t *ec, blkdev_t *dev, blksize_t block);

// Perform a read-modify-write operation for partial write.
// Made for block devices that don't support partial write.
void blkdev_write_partial_fallback(
    badge_err_t   *ec,
    blkdev_t      *dev,
    blksize_t      block,
    size_t         subblock_offset,
    uint8_t const *writebuf,
    size_t         writebuf_len
);
// Read a block into a single-use read cache and copy out part of it.
// Made for block devices that don't support partial read.
void blkdev_read_partial_fallback(
    badge_err_t *ec, blkdev_t *dev, blksize_t block, size_t subblock_offset, uint8_t *readbuf, size_t readbuf_len
);
