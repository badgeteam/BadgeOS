
// SPDX-License-Identifier: MIT

#pragma once

#include "filesystem/vfs_internal.h"

// Inode number of the root directory of a RAM filesystem.
#define VFS_RAMFS_INODE_ROOT  1
// First regular inode of a RAM filesystem.
#define VFS_RAMFS_INODE_FIRST 2

// Try to mount a ramfs filesystem.
void vfs_ramfs_mount(badge_err_t *ec, vfs_t *vfs);
// Unmount a ramfs filesystem.
void vfs_ramfs_umount(vfs_t *vfs);

// Insert a new file into the given directory.
// If `dir` is NULL, the root directory is used.
// If the file already exists, does nothing.
void vfs_ramfs_create_file(badge_err_t *ec, vfs_t *vfs, vfs_file_shared_t *dir, char const *name);
// Insert a new directory into the given directory.
// If `dir` is NULL, the root directory is used.
// If the file already exists, does nothing.
void vfs_ramfs_create_dir(badge_err_t *ec, vfs_t *vfs, vfs_file_shared_t *dir, char const *name);
// Unlink a file from the given directory.
// If `dir` is NULL, the root directory is used.
// If this is the last reference to an inode, the inode is deleted.
void vfs_ramfs_unlink(badge_err_t *ec, vfs_t *vfs, vfs_file_shared_t *dir, char const *name);
// Test for the existence of a file in the given directory.
// If `dir` is NULL, the root directory is used.
bool vfs_ramfs_exists(badge_err_t *ec, vfs_t *vfs, vfs_file_shared_t *dir, char const *name);

// Atomically read all directory entries and cache them into the directory handle.
// Refer to `dirent_t` for the structure of the cache.
void vfs_ramfs_dir_read(badge_err_t *ec, vfs_t *vfs, vfs_file_handle_t *dir);
// Atomically read the directory entry with the matching name.
// Returns true if the entry was found.
bool vfs_ramfs_dir_find_ent(badge_err_t *ec, vfs_t *vfs, vfs_file_shared_t *dir, dirent_t *ent, char const *name);

// Open a file handle for the root directory.
void vfs_ramfs_root_open(badge_err_t *ec, vfs_t *vfs, vfs_file_shared_t *file);
// Open a file for reading and/or writing.
void vfs_ramfs_file_open(
    badge_err_t *ec, vfs_t *vfs, vfs_file_shared_t *dir, vfs_file_shared_t *file, char const *name
);
// Close a file opened by `vfs_ramfs_file_open`.
// Only raises an error if `file` is an invalid file descriptor.
void vfs_ramfs_file_close(badge_err_t *ec, vfs_t *vfs, vfs_file_shared_t *file);
// Read bytes from a file.
void vfs_ramfs_file_read(
    badge_err_t *ec, vfs_t *vfs, vfs_file_shared_t *file, fileoff_t offset, uint8_t *readbuf, fileoff_t readlen
);
// Write bytes from a file.
void vfs_ramfs_file_write(
    badge_err_t *ec, vfs_t *vfs, vfs_file_shared_t *file, fileoff_t offset, uint8_t const *writebuf, fileoff_t writelen
);
// Change the length of a file opened by `vfs_ramfs_file_open`.
void vfs_ramfs_file_resize(badge_err_t *ec, vfs_t *vfs, vfs_file_shared_t *file, fileoff_t new_size);

// Commit all pending writes to disk.
// The filesystem, if it does caching, must always sync everything to disk at once.
void vfs_ramfs_flush(badge_err_t *ec, vfs_t *vfs);
