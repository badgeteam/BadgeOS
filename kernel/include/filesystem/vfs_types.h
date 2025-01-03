
// SPDX-License-Identifier: MIT

#pragma once

// VFS shared opened file handle.
// Shared between all file handles referring to the same file.
typedef struct vfs_file_obj  vfs_file_obj_t;
// VFS opened file handle.
typedef struct vfs_file_desc vfs_file_desc_t;
// VFS mounted filesystem.
typedef struct vfs           vfs_t;

#include "blockdevice.h"
#include "filesystem.h"
#include "filesystem/vfs_vtable.h"
#include "mutex.h"


// VFS shared opened file handle.
// Shared between all file handles referring to the same file.
struct vfs_file_obj {
    // Reference count (how many `vfs_file_desc_t` reference this).
    atomic_int refcount;
    // Index in the shared file handle table.
    ptrdiff_t  index;
    // Current file size.
    fileoff_t  size;
    // Filesystem-specific information.
    void      *cookie;
    // Type of file this is.
    filetype_t type;
    // Inode number (gauranteed to be unique per VFS).
    // No file or directory may have the same inode number.
    // Any file or directory is required to name an inode number of 1 or higher.
    inode_t    inode;
    // Pointer to the VFS on which this file exists.
    vfs_t     *vfs;
    // Handle mutex for concurrency.
    mutex_t    mutex;
};

// VFS opened file handle.
struct vfs_file_desc {
    // Reference count (how many threads reference this).
    atomic_int refcount;
    // Current access position.
    // Note: Must be bounds-checked on every file I/O.
    fileoff_t  offset;
    // File is writeable.
    bool       write;
    // File is readable.
    bool       read;
    // Handle is in append mode.
    bool       append;

    // Pointer to shared file handle.
    // Directories do not have a shared handle.
    vfs_file_obj_t *obj;
    // Handle number.
    file_t          fileno;
};

// VFS mounted filesystem.
struct vfs {
    // Filesystem vtable.
    vfs_vtable_t const *vtable;
    // Copy of mount point.
    char               *mountpoint;
    // Read-only flag.
    bool                readonly;
    // Associated block device.
    blkdev_t           *media;
    // Filesystem type.
    fs_driver_t const  *driver;
    // Inode number given to the root directory.
    inode_t             inode_root;
    // Filesystem-specific information.
    void               *cookie;
};

// Filesystem implementation info.
typedef struct {
    // Filesystem ID.
    char const         *id;
    // Filesystem vtable.
    vfs_vtable_t const *vtable;
    // Size to allocate for the VFS cookie.
    size_t              vfs_cookie_size;
    // Size to allocate for the file handle cookie.
    size_t              file_cookie_size;
} fs_driver_t;

// Declare a filesystem driver.
#define FS_DRIVER_DECL(ident) __attribute__((section(".fsdrivers"))) static fs_driver_t const ident
