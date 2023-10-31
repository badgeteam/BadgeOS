
// SPDX-License-Identifier: MIT

#pragma once

#include "filesystem.h"
#include "filesystem/vfs_types.h"
#include "mutex.h"

#define VFS_MUTEX_TIMEOUT 1500000

// Index in the VFS table of the filesystem mounted at /.
// Set to -1 if no filesystem is mounted at /.
// If no filesystem is mounted at /, the FS API will not work.
extern ptrdiff_t vfs_root_index;
// Table of mounted filesystems.
extern vfs_t     vfs_table[FILESYSTEM_MOUNT_MAX];
// Mutex for filesystem mounting / unmounting.
// Taken exclusively during mount / unmount operations.
// Taken shared during filesystem access.
extern mutex_t   vfs_mount_mtx;
// Mutex for creating and destroying directory and file handles.
// Taken exclusively when a handle is created or destroyed.
// Taken shared when a handle is used.
extern mutex_t   vfs_handle_mtx;

// List of open shared file handles.
extern vfs_file_shared_t **vfs_file_shared_list;
// Number of open shared file handles.
extern size_t              vfs_file_shared_list_len;
// Capacity of open shared file handles list.
extern size_t              vfs_file_shared_list_cap;

// List of open file handles.
extern vfs_file_handle_t *vfs_file_handle_list;
// Number of open file handles.
extern size_t             vfs_file_handle_list_len;
// Capacity of open file handles list.
extern size_t             vfs_file_handle_list_cap;



/* ==== Thread-unsafe functions ==== */

// Find a shared file handle by inode, if any.
ptrdiff_t vfs_shared_by_inode(vfs_t *vfs, inode_t inode);
// Get a file handle by number.
ptrdiff_t vfs_file_by_handle(file_t fileno);
// Create a new empty shared file handle.
ptrdiff_t vfs_file_create_shared();
// Destroy a shared file handle assuming the underlying file is already closed.
void      vfs_file_destroy_shared(ptrdiff_t shared);
// Create a new file handle.
// If `shared` is -1, a new shared empty handle is created.
ptrdiff_t vfs_file_create_handle(ptrdiff_t shared);
// Delete a file handle.
// If this is the last handle referring to one file, the shared handle is closed too.
void      vfs_file_destroy_handle(ptrdiff_t handle);



/* ==== Thread-safe functions ==== */

// Open the root directory of the root filesystem.
void vfs_root_open(badge_err_t *ec, vfs_file_shared_t *dir);

// Insert a new file into the given directory.
// If the file already exists, does nothing.
void vfs_create_file(badge_err_t *ec, vfs_file_shared_t *dir, char const *name);
// Insert a new directory into the given directory.
// If the file already exists, does nothing.
void vfs_create_dir(badge_err_t *ec, vfs_file_shared_t *dir, char const *name);
// Unlink a file from the given directory.
// If this is the last reference to an inode, the inode is deleted.
void vfs_unlink(badge_err_t *ec, vfs_file_shared_t *dir, char const *name);

// Atomically read all directory entries and cache them into the directory handle.
// Refer to `dirent_t` for the structure of the cache.
void vfs_dir_read(badge_err_t *ec, vfs_file_handle_t *dir);
// Atomically read the directory entry with the matching name.
// Returns true if the entry was found.
bool vfs_dir_find_ent(badge_err_t *ec, vfs_file_shared_t *dir, dirent_t *ent, char const *name);

// Open a file for reading and/or writing given parent directory handle.
// Also handles OFLAGS_EXCLUSIVE and OFLAGS_CREATE.
void vfs_file_open(badge_err_t *ec, vfs_file_shared_t *dir, vfs_file_shared_t *file, char const *name, oflags_t oflags);
// Close a file opened by `vfs_file_open`.
// Only raises an error if `file` is an invalid file descriptor.
void vfs_file_close(badge_err_t *ec, vfs_file_shared_t *file);
// Read bytes from a file.
// The entire read succeeds or the entire read fails, never partial read.
void vfs_file_read(badge_err_t *ec, vfs_file_shared_t *file, fileoff_t offset, uint8_t *readbuf, fileoff_t readlen);
// Write bytes to a file.
// If the file is not large enough, it expanded.
// The entire write succeeds or the entire write fails, never partial write.
void vfs_file_write(
    badge_err_t *ec, vfs_file_shared_t *file, fileoff_t offset, uint8_t const *writebuf, fileoff_t writelen
);
// Change the length of a file opened by `vfs_file_open`.
void vfs_file_resize(badge_err_t *ec, vfs_file_shared_t *file, fileoff_t new_size);

// Commit all pending writes to disk.
// The filesystem, if it does caching, must always sync everything to disk at once.
void vfs_flush(badge_err_t *ec, vfs_t *vfs);
