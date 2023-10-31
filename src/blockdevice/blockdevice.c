
// SPDX-License-Identifier: MIT

#include "blockdevice.h"

#include "badge_strings.h"
#include "blockdevice/blkdev_internal.h"
#include "blockdevice/blkdev_ram.h"
#include "log.h"
#include "malloc.h"



// Test whether a cache entry is dirty.
static bool blkdev_is_dirty(blkdev_flags_t) __attribute__((const));
static bool blkdev_is_dirty(blkdev_flags_t flags) {
    return flags.present && (flags.erase || flags.dirty);
}

// Flush a cache entry.
static void blkdev_flush_cache(badge_err_t *ec, blkdev_t *dev, size_t i) {
    uint8_t        *cache = dev->cache->block_cache;
    blkdev_flags_t *flags = dev->cache->block_flags;

    // Clear dirty flags before operation for concurrency.
    bool dirty     = flags[i].dirty;
    bool erase     = flags[i].erase;
    flags[i].dirty = false;
    flags[i].erase = false;

    if (dirty) {
        // Only keep the entry if read caching is enabled.
        flags[i].present = dev->cache_read;
        if (dev->cache_read) {
            flags[i].update_time = time_us();
        }
        blkdev_write_raw(ec, dev, flags[i].index, cache + (i * dev->block_size));
    } else if (erase) {
        // If it is erased, discard the cache entry.
        flags[i].present = false;
        blkdev_erase_raw(ec, dev, flags[i].index);
    }
}

// Find an empty cache entry.
// Returns -1 when not found.
static inline ptrdiff_t blkdev_alloc_cache(blkdev_t *dev, blksize_t block) __attribute__((pure));
static inline ptrdiff_t blkdev_alloc_cache(blkdev_t *dev, blksize_t block) {
    if (!dev->cache)
        return -1;

    blkdev_flags_t *flags = dev->cache->block_flags;

    timestamp_us_t oldest_read_time  = time_us() - BLKDEV_READ_CACHE_TIMEOUT;
    ptrdiff_t      oldest_read_index = -1;

    for (size_t i = 0; i < dev->cache->cache_depth; i++) {
        if (!flags[i].present || flags[i].index == block) {
            // Vacant cache entry found.
            return i;
        } else if (!blkdev_is_dirty(flags[i]) && oldest_read_time > flags[i].update_time) {
            // Sufficiently old read cache entry found.
            oldest_read_time  = flags[i].update_time;
            oldest_read_index = i;
        }
    }

    if (oldest_read_index >= 0) {
        // Only the oldest read cache entry is replaced.
        return oldest_read_index;
    }

    return -1;
}

// Find the cache entry for a certain block.
// Returns -1 when not found.
static inline ptrdiff_t blkdev_find_cache(blkdev_t *dev, blksize_t block) __attribute__((pure));
static inline ptrdiff_t blkdev_find_cache(blkdev_t *dev, blksize_t block) {
    if (!dev->cache)
        return -1;

    blkdev_flags_t *flags = dev->cache->block_flags;

    for (size_t i = 0; i < dev->cache->cache_depth; i++) {
        if (flags[i].present && flags[i].index == block) {
            return i;
        }
    }

    return -1;
}



// Write without caching.
void blkdev_write_raw(badge_err_t *ec, blkdev_t *dev, blksize_t block, uint8_t const *writebuf) {
    switch (dev->type) {
        case BLKDEV_TYPE_RAM: return blkdev_ram_write(ec, dev, block, writebuf);
        // case BLKDEV_TYPE_I2C_EEPROM: return blkdev_i2c_eeprom_write(ec, dev, block, writebuf);
        default: badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM); break;
    }
}

// Read without caching.
void blkdev_read_raw(badge_err_t *ec, blkdev_t *dev, blksize_t block, uint8_t *readbuf) {
    switch (dev->type) {
        case BLKDEV_TYPE_RAM: return blkdev_ram_read(ec, dev, block, readbuf);
        // case BLKDEV_TYPE_I2C_EEPROM: return blkdev_i2c_eeprom_read(ec, dev, block, readbuf);
        default: badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM); break;
    }
}

// Erase without caching.
void blkdev_erase_raw(badge_err_t *ec, blkdev_t *dev, blksize_t block) {
    switch (dev->type) {
        case BLKDEV_TYPE_RAM: return blkdev_ram_erase(ec, dev, block);
        // case BLKDEV_TYPE_I2C_EEPROM: return blkdev_i2c_eeprom_erase(ec, dev, block);
        default: badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM); break;
    }
}

// Read a block into a single-use read cache and copy out part of it.
// Made for block devices that don't support partial read.
void blkdev_write_partial_fallback(
    badge_err_t   *ec,
    blkdev_t      *dev,
    blksize_t      block,
    size_t         subblock_offset,
    uint8_t const *writebuf,
    size_t         writebuf_len
) {
    uint8_t        *cache = dev->cache->block_cache;
    blkdev_flags_t *flags = dev->cache->block_flags;

    // Try to look up the block in the cache.
    ptrdiff_t i = blkdev_find_cache(dev, block);
    if (i >= 0) {
        // Update the entry in the cache.
        flags[i].erase = false;
        flags[i].dirty = false;
        if (!flags[i].present) {
            flags[i].present     = true;
            flags[i].update_time = time_us();
            flags[i].index       = block;
        }
        // Copy data into the cache entry.
        mem_copy(cache + i * dev->block_size + subblock_offset, writebuf, writebuf_len);
        badge_err_set_ok(ec);

    } else {
        // Cache was not available, allocate some on the heap.
        uint8_t *tmp = malloc(dev->block_size);
        if (!tmp) {
            badge_err_set(ec, ELOC_BLKDEV, ECAUSE_NOMEM);
            return;
        }
        // Read into the temporary buffer.
        badge_err_t ec0;
        if (!ec)
            ec = &ec0;
        blkdev_read_raw(ec, dev, block, tmp);
        if (!badge_err_is_ok(ec)) {
            free(tmp);
            return;
        }
        // Modify and write back.
        mem_copy(tmp + subblock_offset, writebuf, writebuf_len);
        blkdev_write_raw(ec, dev, block, tmp);
        free(tmp);
    }
}

// Perform a read-modify-write operation for partial write.
// Made for block devices that don't support partial write.
void blkdev_read_partial_fallback(
    badge_err_t *ec, blkdev_t *dev, blksize_t block, size_t subblock_offset, uint8_t *readbuf, size_t readbuf_len
) {
    uint8_t        *cache = dev->cache->block_cache;
    blkdev_flags_t *flags = dev->cache->block_flags;

    // Try to look up the block in the cache.
    ptrdiff_t i = blkdev_alloc_cache(dev, block);
    if (i >= 0 && flags[i].present) {
        // Existing cache found, copy it out.
        mem_copy(readbuf, cache + i * dev->block_size + subblock_offset, readbuf_len);
        // If it is a read cache, reset the timeout.
        if (!blkdev_is_dirty(flags[i])) {
            flags[i].update_time = time_us();
        }
        badge_err_set_ok(ec);

    } else if (i >= 0) {
        // Empty cache entry found.
        badge_err_t ec0;
        if (!ec)
            ec = &ec0;
        // Create read cache regardless of read cache enable.
        blkdev_read_raw(ec, dev, block, cache + i * dev->block_offset);
        flags[i] = (blkdev_flags_t){
            .update_time = time_us(),
            .index       = block,
            .present     = true,
            .erase       = false,
            .dirty       = false,
        };

        // Partial copy.
        mem_copy(readbuf, cache + i * dev->block_offset + subblock_offset, readbuf_len);

    } else {
        // Cache was not available, allocate some on the heap.
        uint8_t *tmp = malloc(dev->block_size);
        if (!tmp) {
            badge_err_set(ec, ELOC_BLKDEV, ECAUSE_NOMEM);
            return;
        }
        // Read into the temporary buffer.
        badge_err_t ec0;
        if (!ec)
            ec = &ec0;
        blkdev_read_raw(ec, dev, block, tmp);
        if (!badge_err_is_ok(ec)) {
            free(tmp);
            return;
        }
        // Partial copy.
        mem_copy(readbuf, tmp + subblock_offset, readbuf_len);
        free(tmp);
    }
}



// Prepare a block device for reading and/or writing.
// For some block devices, this may allocate caches.
void blkdev_open(badge_err_t *ec, blkdev_t *dev) {
    if (!dev) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM);
        return;
    }
    switch (dev->type) {
        case BLKDEV_TYPE_RAM: return blkdev_ram_open(ec, dev);
        // case BLKDEV_TYPE_I2C_EEPROM: return blkdev_i2c_eeprom_open(ec, dev);
        default: badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM); break;
    }
}

// Flush write caches and close block device.
void blkdev_close(badge_err_t *ec, blkdev_t *dev) {
    if (!dev) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM);
        return;
    }
    blkdev_flush(ec, dev);
    if (!badge_err_is_ok(ec))
        return;
    switch (dev->type) {
        case BLKDEV_TYPE_RAM: return blkdev_ram_close(ec, dev);
        // case BLKDEV_TYPE_I2C_EEPROM: return blkdev_i2c_eeprom_close(ec, dev);
        default: badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM); break;
    }
}



// Query the erased status of a block.
// On devices which cannot erase blocks, this will always return true.
// Returns true on error.
bool blkdev_is_erased(badge_err_t *ec, blkdev_t *dev, blksize_t block) {
    if (!dev) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM);
        return true;
    }
    if (dev->readonly) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_READONLY);
        return true;
    }

    // Check cache before querying hardware.
    ptrdiff_t i = blkdev_find_cache(dev, block);
    if (i >= 0) {
        return dev->cache->block_flags[i].erase;
    }

    switch (dev->type) {
        case BLKDEV_TYPE_RAM: return blkdev_ram_is_erased(ec, dev, block);
        // case BLKDEV_TYPE_I2C_EEPROM: return blkdev_i2c_eeprom_is_erased(ec, dev, block);
        default: badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM); return true;
    }
}

// Explicitly erase a block, if possible.
// On devices which cannot erase blocks, this will do nothing.
void blkdev_erase(badge_err_t *ec, blkdev_t *dev, blksize_t block) {
    if (!dev) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM);
        return;
    }
    if (dev->readonly) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_READONLY);
        return;
    }

    blkdev_flags_t *flags = dev->cache->block_flags;

    // Attempt to cache erase operation.
    ptrdiff_t i = blkdev_alloc_cache(dev, block);
    if (i >= 0) {
        if (flags[i].present) {
            // Set flag in existing cache entry.
            flags[i].erase = true;
            flags[i].dirty = false;
        } else {
            // Set flag in new cache entry.
            flags[i] = (blkdev_flags_t){
                .update_time = time_us(),
                .index       = block,
                .present     = true,
                .erase       = false,
                .dirty       = true,
            };
        }
        badge_err_set_ok(ec);

    } else {
        // Uncached or out of free cache.
        blkdev_erase_raw(ec, dev, block);
    }
}

// Erase if necessary and write a block.
// This operation may be cached and therefor delayed.
void blkdev_write(badge_err_t *ec, blkdev_t *dev, blksize_t block, uint8_t const *writebuf) {
    if (!dev) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM);
        return;
    }
    if (dev->readonly) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_READONLY);
        return;
    }

    uint8_t        *cache = dev->cache->block_cache;
    blkdev_flags_t *flags = dev->cache->block_flags;

    // Attempt to cache write operation.
    ptrdiff_t i = blkdev_alloc_cache(dev, block);
    if (i >= 0) {
        if (flags[i].present) {
            // Set flag in existing cache entry.
            flags[i].erase = false;
            flags[i].dirty = true;
        } else {
            // Set flag in new cache entry.
            flags[i] = (blkdev_flags_t){
                .update_time = time_us(),
                .index       = block,
                .present     = true,
                .erase       = false,
                .dirty       = true,
            };
        }
        mem_copy(cache + i * dev->block_size, writebuf, dev->block_size);
        badge_err_set_ok(ec);

    } else {
        // Uncached or out of free cache.
        blkdev_write_raw(ec, dev, block, writebuf);
    }
}

// Read a block.
// This operation may be cached.
void blkdev_read(badge_err_t *ec, blkdev_t *dev, blksize_t block, uint8_t *readbuf) {
    if (!dev) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM);
        return;
    }

    uint8_t        *cache = dev->cache->block_cache;
    blkdev_flags_t *flags = dev->cache->block_flags;

    // Look for the entry in the cache.
    ptrdiff_t i = blkdev_alloc_cache(dev, block);
    if (i >= 0 && flags[i].present) {
        // Existing cache entry.
        if (flags[i].erase) {
            mem_set(readbuf, 255, dev->block_size);
        } else {
            if (!flags[i].dirty) {
                flags[i].update_time = time_us();
            }
            mem_copy(readbuf, cache + i * dev->block_size, dev->block_size);
        }
        badge_err_set_ok(ec);

    } else if (i >= 0 && dev->cache_read) {
        // Read caching is enabled.
        badge_err_t ec0;
        if (!ec)
            ec = &ec0;
        blkdev_read_raw(ec, dev, block, cache + i * dev->block_size);

        flags[i] = (blkdev_flags_t){
            .update_time = time_us(),
            .index       = block,
            .present     = true,
            .erase       = false,
            .dirty       = false,
        };

        if (!badge_err_is_ok(ec))
            return;
        mem_copy(readbuf, cache + i * dev->block_size, dev->block_size);
    } else {
        // Uncached or out of free cache.
        blkdev_read_raw(ec, dev, block, readbuf);
    }
}

// Partially write a block.
// This is very likely to cause a read-modify-write operation.
void blkdev_write_partial(
    badge_err_t   *ec,
    blkdev_t      *dev,
    blksize_t      block,
    size_t         subblock_offset,
    uint8_t const *writebuf,
    size_t         writebuf_len
) {
    if (!dev) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM);
        return;
    }
    if (dev->readonly) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_READONLY);
        return;
    }

    // Perform some bounds checking.
    if (writebuf_len == 0) {
        badge_err_set_ok(ec);
        return;
    }
    if (subblock_offset >= dev->block_size || writebuf_len > dev->block_size ||
        subblock_offset + writebuf_len > dev->block_size) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_RANGE);
        return;
    }

    switch (dev->type) {
        case BLKDEV_TYPE_RAM: return blkdev_ram_write_partial(ec, dev, block, subblock_offset, writebuf, writebuf_len);
        // case BLKDEV_TYPE_I2C_EEPROM:
        //     return blkdev_i2c_eeprom_write_partial(ec, dev, block, subblock_offset, writebuf, writebuf_len);
        default: badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM); break;
    }
}

// Partially read a block.
// This may use read caching if the device doesn't support partial read.
void blkdev_read_partial(
    badge_err_t *ec, blkdev_t *dev, blksize_t block, size_t subblock_offset, uint8_t *readbuf, size_t readbuf_len
) {
    if (!dev) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM);
        return;
    }

    // Perform some bounds checking.
    if (readbuf_len == 0) {
        badge_err_set_ok(ec);
        return;
    }
    if (subblock_offset >= dev->block_size || readbuf_len > dev->block_size ||
        subblock_offset + readbuf_len > dev->block_size) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_RANGE);
        return;
    }

    switch (dev->type) {
        case BLKDEV_TYPE_RAM: return blkdev_ram_read_partial(ec, dev, block, subblock_offset, readbuf, readbuf_len);
        // case BLKDEV_TYPE_I2C_EEPROM:
        //     return blkdev_i2c_eeprom_read_partial(ec, dev, block, subblock_offset, readbuf, readbuf_len);
        default: badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM); break;
    }
}



// Flush the write cache to the block device.
void blkdev_flush(badge_err_t *ec, blkdev_t *dev) {
    if (!dev) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM);
        return;
    }
    if (dev->readonly) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_READONLY);
        return;
    }

    blkdev_flags_t *flags = dev->cache->block_flags;

    badge_err_t ec0;
    if (!ec)
        ec = &ec0;

    for (size_t i = 0; i < dev->cache->cache_depth; i++) {
        if (flags[i].present && (flags[i].dirty || flags[i].erase)) {
            blkdev_flush_cache(ec, dev, i);
            if (ec)
                return;
        }
    }
}

// Call this function occasionally per block device to do housekeeping.
// Manages flushing of caches and erasure.
void blkdev_housekeeping(badge_err_t *ec, blkdev_t *dev) {
    timestamp_us_t now     = time_us();
    timestamp_us_t timeout = now - BLKDEV_WRITE_CACHE_TIMEOUT;

    blkdev_flags_t *flags = dev->cache->block_flags;

    badge_err_t ec0;
    if (!ec)
        ec = &ec0;

    for (size_t i = 0; i < dev->cache->cache_depth; i++) {
        if (blkdev_is_dirty(flags[i]) && flags[i].update_time < timeout) {
            blkdev_flush_cache(ec, dev, i);
            if (!badge_err_is_ok(ec))
                return;
        }
    }
}

// Allocate a cache for a block device.
void blkdev_create_cache(badge_err_t *ec, blkdev_t *dev, size_t cache_depth) {
    if (!dev) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM);
        return;
    }
    if (dev->readonly && !dev->cache_read) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_READONLY);
        return;
    }
    if (dev->cache) {
        // If cache is already present, flush and remove it first.
        badge_err_t ec0;
        if (!ec)
            ec = &ec0;
        blkdev_flush(ec, dev);
        if (!badge_err_is_ok(ec))
            return;
    }

    // Allocate cache info.
    dev->cache = malloc(sizeof(blkdev_cache_t));
    if (!dev->cache) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_NOMEM);
        return;
    }
    dev->cache->cache_depth = cache_depth;

    // Allocate block cache.
    dev->cache->block_cache = malloc(dev->block_size * cache_depth);
    if (!dev->cache->block_cache) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_NOMEM);
        free(dev->cache);
        dev->cache = NULL;
        return;
    }

    // Allocate block flags.
    dev->cache->block_flags = malloc(sizeof(blkdev_flags_t) * cache_depth);
    if (!dev->cache->block_flags) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_NOMEM);
        free(dev->cache->block_cache);
        free(dev->cache);
        dev->cache = NULL;
        return;
    }
    mem_set(dev->cache->block_flags, 0, sizeof(blkdev_flags_t) * cache_depth);
    badge_err_set_ok(ec);
}

// Remove a cache from a block device.
void blkdev_delete_cache(badge_err_t *ec, blkdev_t *dev) {
    if (!dev) {
        badge_err_set(ec, ELOC_BLKDEV, ECAUSE_PARAM);
        return;
    }
    if (dev->cache) {
        badge_err_t ec0;
        if (!ec)
            ec = &ec0;
        blkdev_flush(ec, dev);
        if (!badge_err_is_ok(ec))
            return;

        free(dev->cache->block_cache);
        free(dev->cache->block_flags);
        free(dev->cache);
        dev->cache = NULL;
    }
    badge_err_set_ok(ec);
}



// Show a summary of the cache entries.
void blkdev_dump_cache(blkdev_t *dev) {
    if (!dev)
        return;

    if (!dev->cache) {
        logk(LOG_DEBUG, "BLKDEV: uncached");
        return;
    }

    size_t used = 0;
    for (size_t i = 0; i < dev->cache->cache_depth; i++) {
        if (dev->cache->block_flags[i].present) {
            used++;
        }
    }

    logkf(LOG_DEBUG, "BLKDEV: %{size;d} cache %{cs} used:", used, used == 1 ? "entry" : "entries");

    for (size_t i = 0; i < dev->cache->cache_depth; i++) {
        if (dev->cache->block_flags[i].present) {
            if (dev->cache->block_flags[i].dirty) {
                logkf(
                    LOG_DEBUG,
                    "BLKDEV: Entry %{size;d}: block %{u32;d} write cache",
                    i,
                    dev->cache->block_flags[i].index
                );
            } else if (dev->cache->block_flags[i].erase) {
                logkf(
                    LOG_DEBUG,
                    "BLKDEV: Entry %{size;d}: block %{u32;d} erase cache",
                    i,
                    dev->cache->block_flags[i].index
                );
            } else {
                logkf(
                    LOG_DEBUG,
                    "BLKDEV: Entry %{size;d}: block %{u32;d} read  cache",
                    i,
                    dev->cache->block_flags[i].index
                );
            }
        }
    }
}
