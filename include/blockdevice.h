
// SPDX-License-Identifier: MIT

#pragma once

#include "badge_err.h"
#include "time.h"

#include <stddef.h>

// Maximum age in microseconds of a write cache entry.
#define BLKDEV_WRITE_CACHE_TIMEOUT 1000000
// Minimum age in microseconds of a read cache entry.
#define BLKDEV_READ_CACHE_TIMEOUT  1000000

// Size type used for block devices.
typedef uint32_t blksize_t;
// Offset type used for block devices.
typedef int32_t  blkoff_t;

// Supported block device types.
typedef enum {
    // RAM virtual block device.
    BLKDEV_TYPE_RAM,
    // I²C EEPROM block device.
    BLKDEV_TYPE_I2C_EEPROM,
} blkdev_type_t;

// Address for RAM virtual block devices.
typedef void    *blkdev_ram_addr_t;
// Address for I²C EEPROM block devices.
typedef uint16_t blkdev_i2c_eeprom_addr_t;

// Flags and auxiliary data used for the cache.
typedef struct {
    // Write/erase: Timestamp of most recent sync to disk.
    // Read: Timestamp of most recent access.
    // If a cached read happens on a dirty or erase entry, the timestamp is not changed.
    timestamp_us_t update_time;
    // Block index referred to.
    blksize_t      index;
    // Cache entry contains data.
    bool           present;
    // Block is marked for erasure.
    bool           erase;
    // Cache entry differs from disk.
    bool           dirty;
} blkdev_flags_t;

// Block device cache data.
typedef struct {
    // Pointer to block cache memory.
    // Must be large enough for `cache_depth` blocks.
    uint8_t        *block_cache;
    // Pointer to block flags memory.
    // Must be large enough for `cache_depth` entries.
    blkdev_flags_t *block_flags;
    // Amount of cache entries.
    size_t          cache_depth;
} blkdev_cache_t;

// Block device descriptor.
typedef struct {
    // Type of block device.
    blkdev_type_t type;
    // Physical block size.
    blksize_t     block_size;
    // Physical block index of the first logical block.
    blksize_t     block_offset;
    // Number of blocks from this device to use.
    blksize_t     blocks;
    // Is a read-only block device.
    bool          readonly;
    // Block device address.
    union {
        blkdev_ram_addr_t        ram_addr;
        blkdev_i2c_eeprom_addr_t i2c_eeprom_addr;
    };
    // If `cache` is nonnull: enable a read cache.
    bool            cache_read;
    // Write and optionally read cache.
    // This may be statically allocated, or created using `blkdev_create_cache`.
    blkdev_cache_t *cache;
} blkdev_t;

// Prepare a block device for reading and/or writing.
// All other `blkdev_*` functions assume the block device was opened using this function.
// For some block devices, this may allocate caches.
void blkdev_open(badge_err_t *ec, blkdev_t *dev);
// Flush write caches and close block device.
void blkdev_close(badge_err_t *ec, blkdev_t *dev);

// Query the erased status of a block.
// On devices which cannot erase blocks, this will always return true.
// Returns true on error.
bool blkdev_is_erased(badge_err_t *ec, blkdev_t *dev, blksize_t block);
// Explicitly erase a block, if possible.
// On devices which cannot erase blocks, this will do nothing.
void blkdev_erase(badge_err_t *ec, blkdev_t *dev, blksize_t block);
// Erase if necessary and write a block.
// This operation may be cached and therefor delayed.
void blkdev_write(badge_err_t *ec, blkdev_t *dev, blksize_t block, uint8_t const *writebuf);
// Read a block.
// This operation may be cached.
void blkdev_read(badge_err_t *ec, blkdev_t *dev, blksize_t block, uint8_t *readbuf);
// Partially write a block.
// This is very likely to cause a read-modify-write operation.
void blkdev_write_partial(
    badge_err_t   *ec,
    blkdev_t      *dev,
    blksize_t      block,
    size_t         subblock_offset,
    uint8_t const *writebuf,
    size_t         writebuf_len
);
// Partially read a block.
// This may use read caching if the device doesn't support partial read.
void blkdev_read_partial(
    badge_err_t *ec, blkdev_t *dev, blksize_t block, size_t subblock_offset, uint8_t *readbuf, size_t readbuf_len
);

// Flush the write cache to the block device.
void blkdev_flush(badge_err_t *ec, blkdev_t *dev);
// Call this function occasionally per block device to do housekeeping.
// Manages flushing of caches and erasure.
void blkdev_housekeeping(badge_err_t *ec, blkdev_t *dev);
// Allocate a cache for a block device.
void blkdev_create_cache(badge_err_t *ec, blkdev_t *dev, size_t cache_depth);
// Remove a cache from a block device.
void blkdev_delete_cache(badge_err_t *ec, blkdev_t *dev);

// Show a summary of the cache entries.
void blkdev_dump_cache(blkdev_t *dev);
