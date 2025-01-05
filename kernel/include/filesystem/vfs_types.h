
// SPDX-License-Identifier: MIT

#pragma once

// VFS shared opened file handle.
// Shared between all file handles referring to the same file.
typedef struct vfs_file_obj  vfs_file_obj_t;
// VFS opened file handle.
typedef struct vfs_file_desc vfs_file_desc_t;
// VFS mounted filesystem.
typedef struct vfs           vfs_t;
// Filesystem implementation info.
typedef struct fs_driver     fs_driver_t;

#include "blockdevice.h"
#include "filesystem.h"
#include "filesystem/vfs_vtable.h"
#include "mutex.h"


// VFS shared opened file handle.
// Shared between all file handles referring to the same file.
struct vfs_file_obj {
    // Reference count (how many `vfs_file_desc_t` reference this).
    atomic_int refcount;
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
    // FS mounted in this directory, if any.
    vfs_t     *mounted_fs;
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
    vfs_vtable_t       vtable;
    // Copy of mount point.
    char              *mountpoint;
    // Read-only flag.
    bool               readonly;
    // Associated block device.
    blkdev_t          *media;
    // Filesystem type.
    fs_driver_t const *driver;
    // Inode number given to the root directory.
    inode_t            inode_root;
    // Filesystem-specific information.
    void              *cookie;
    // Number of currently open files.
    atomic_int         n_open_fd;
    // Roor directory file object.
    vfs_file_obj_t    *root_dir_obj;
};

// Identify whether a block device contains a this filesystem.
// Returns false on error.
typedef bool (*vfs_detect_t)(badge_err_t *ec, blkdev_t *dev);

// Filesystem implementation info.
struct fs_driver {
    // Filesystem ID.
    char const         *id;
    // Filesystem vtable.
    vfs_vtable_t const *vtable;
    // Filesystem detection function.
    vfs_detect_t        detect;
    // Size to allocate for the VFS cookie.
    size_t              vfs_cookie_size;
    // Size to allocate for the file handle cookie.
    size_t              file_cookie_size;
};

// Declare a filesystem driver.
#define FS_DRIVER_DECL(ident) __attribute__((used, section(".fsdrivers"))) static fs_driver_t const ident
