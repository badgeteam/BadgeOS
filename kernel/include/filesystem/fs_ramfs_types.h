
// SPDX-License-Identifier: MIT

#pragma once

#include "assertions.h"
#include "attributes.h"
#include "filesystem.h"
#include "mutex.h"

#include <stdatomic.h>

// Maximum name length of RAMFS.
#define VFS_RAMFS_NAME_MAX 255
#if FILESYSTEM_NAME_MAX < VFS_RAMFS_NAME_MAX
#undef VFS_RAMFS_NAME_MAX
#define VFS_RAMFS_NAME_MAX FILESYSTEM_NAME_MAX
#endif

// Bit position of file type in mode field.
#define VFS_RAMFS_MODE_BIT  12
// Bit mask of file type in mode field.
#define VFS_RAMFS_MODE_MASK 0xf000



/* ==== In-memory structures ==== */

// File data storage.
typedef struct {
    // Data buffer length.
    size_t   len;
    // Data buffer capacity.
    size_t   cap;
    // Data buffer.
    char    *buf;
    // Inode number.
    inode_t  inode;
    // File type and protection.
    uint16_t mode;
    // Number of hard links.
    size_t   links;
    // Owner user ID.
    int      uid;
    // Owner group ID.
    int      gid;
} fs_ramfs_inode_t;

// RAMFS directory entry.
typedef struct {
    // Entry size.
    size_t  size;
    // Inode number.
    inode_t inode;
    // Name length.
    size_t  name_len;
    // Filename.
    char    name[VFS_RAMFS_NAME_MAX + 1];
} fs_ramfs_dirent_t;

// RAM filesystem file / directory handle.
// This handle is shared between multiple holders of the same file.
typedef fs_ramfs_inode_t *fs_ramfs_file_t;

// Mounted RAM filesystem.
typedef struct {
    // RAM limit for the entire filesystem.
    size_t            ram_limit;
    // RAM usage.
    atomic_size_t     ram_usage;
    // Inode table, indices 0 and 1 are unused.
    fs_ramfs_inode_t *inode_list;
    // Inode table usage map.
    bool             *inode_usage;
    // Number of allocated inodes.
    size_t            inode_list_len;
    // THE RAMFS mutex.
    // Acquired shared for all read-only operations.
    // Acquired exclusive for any write operation.
    mutex_t           mtx;
} fs_ramfs_t;
