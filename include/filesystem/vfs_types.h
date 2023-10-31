
// SPDX-License-Identifier: MIT

#pragma once

#include "blockdevice.h"
#include "filesystem.h"
#include "filesystem/vfs_fat_types.h"
#include "filesystem/vfs_ramfs_types.h"
#include "mutex.h"

typedef struct vfs vfs_t;

// VFS shared opened file handle.
// Shared between all file handles referring to the same file.
typedef struct {
    // Reference count.
    size_t    refcount;
    // Index in the shared file handle table.
    ptrdiff_t index;
    // Current file size.
    fileoff_t size;
    // Filesystem-specific information.
    union {
        // RAMFS.
        vfs_ramfs_file_t ramfs_file;
        // FAT12, FAT16 or FAT32.
        vfs_fat_file_t   fat_file;
    };

    // Cached region offset.
    fileoff_t cache_off;
    // Cached region size.
    fileoff_t cache_size;
    // Cached register buffer.
    char     *cache;

    // Inode number (gauranteed to be unique per VFS).
    // No file or directory may have the same inode number.
    // Any file is required to name an inode number of 3 or higher.
    inode_t inode;
    // Pointer to the VFS on which this file exists.
    vfs_t  *vfs;
} vfs_file_shared_t;

// VFS opened file handle.
typedef struct {
    // Current access position.
    // Note: Must be bounds-checked on every file I/O.
    fileoff_t offset;
    // File is writeable.
    bool      write;
    // File is readable.
    bool      read;
    // Handle refers to a directory.
    bool      is_dir;
    // Handle mutex for concurrency.
    mutex_t   mutex;

    // Directories: Cached size.
    fileoff_t dir_cache_size;
    // Directories: Cache.
    char     *dir_cache;

    // Pointer to shared file handle.
    // Directories do not have a shared handle.
    vfs_file_shared_t *shared;
    // Handle number.
    file_t             fileno;
} vfs_file_handle_t;

// VFS mounted filesystem.
struct vfs {
    // Copy of mount point.
    char     *mountpoint;
    // Read-only flag.
    bool      readonly;
    // Associated block device.
    blkdev_t *media;
    // Filesystem type.
    fs_type_t type;
    // Inode number given to the root directory.
    inode_t   inode_root;
    // Filesystem-specific information.
    union {
        // RAMFS.
        vfs_ramfs_t ramfs;
        // FAT12, FAT16 or FAT32.
        vfs_fat_t   fat;
    };
};
