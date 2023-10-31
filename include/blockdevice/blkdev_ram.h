
// SPDX-License-Identifier: MIT

#include "blockdevice/blkdev_internal.h"

// Connect to the block device if required.
void               blkdev_ram_open(badge_err_t *ec, blkdev_t *dev);
// Disconnect from the block device.
// Writes will already have been flushed to disk.
void               blkdev_ram_close(badge_err_t *ec, blkdev_t *dev);
// Query whether a block is erased.
bool               blkdev_ram_is_erased(badge_err_t *ec, blkdev_t *dev, blksize_t block);
// Erase a block.
void               blkdev_ram_erase(badge_err_t *ec, blkdev_t *dev, blksize_t block);
// Erase if required and write a block.
void               blkdev_ram_write(badge_err_t *ec, blkdev_t *dev, blksize_t block, uint8_t const *writebuf);
// Read a block.
void               blkdev_ram_read(badge_err_t *ec, blkdev_t *dev, blksize_t block, uint8_t *readbuf);
// Partially write a block.
// This is very likely to cause a read-modify-write operation.
static inline void blkdev_ram_write_partial(
    badge_err_t   *ec,
    blkdev_t      *dev,
    blksize_t      block,
    size_t         subblock_offset,
    uint8_t const *writebuf,
    size_t         writebuf_len
) {
    blkdev_write_partial_fallback(ec, dev, block, subblock_offset, writebuf, writebuf_len);
}
// Partially read a block.
// This may use read caching if the device doesn't support partial read.
static inline void blkdev_ram_read_partial(
    badge_err_t *ec, blkdev_t *dev, blksize_t block, size_t subblock_offset, uint8_t *readbuf, size_t readbuf_len
) {
    blkdev_read_partial_fallback(ec, dev, block, subblock_offset, readbuf, readbuf_len);
}
// Check whether caching is recommended.
bool blkdev_ram_do_cache(badge_err_t *ec, blkdev_t *dev);