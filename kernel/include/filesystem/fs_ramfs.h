
// SPDX-License-Identifier: MIT

#pragma once

#include "filesystem/fs_ramfs_types.h"
#include "filesystem/vfs_internal.h"

// Inode number of the root directory of a RAM filesystem.
#define VFS_RAMFS_INODE_ROOT  1
// First regular inode of a RAM filesystem.
#define VFS_RAMFS_INODE_FIRST 2

// Try to mount a ramfs filesystem.
bool fs_ramfs_mount(badge_err_t *ec, vfs_t *vfs);
// Unmount a ramfs filesystem.
void fs_ramfs_umount(vfs_t *vfs);

// Insert a new file into the given directory.
// If `dir` is NULL, the root directory is used.
// If the file already exists, does nothing.
void fs_ramfs_create_file(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, char const *name, size_t name_len);
// Insert a new directory into the given directory.
// If `dir` is NULL, the root directory is used.
// If the file already exists, does nothing.
void fs_ramfs_create_dir(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, char const *name, size_t name_len);
// Unlink a file from the given directory.
// If `dir` is NULL, the root directory is used.
// If this is the last reference to an inode, the inode is deleted.
void fs_ramfs_unlink(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, char const *name, size_t name_len);
// Test for the existence of a file in the given directory.
// If `dir` is NULL, the root directory is used.
bool fs_ramfs_exists(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, char const *name, size_t name_len);

// Read all entries from a directory.
dirent_list_t fs_ramfs_dir_read(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir);

// Read the directory entry with the matching name.
// Returns true if the entry was found.
bool fs_ramfs_dir_find_ent(
    badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, dirent_t *ent, char const *name, size_t name_len
);

// Open a file handle for the root directory.
void fs_ramfs_root_open(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *file);
// Open a file for reading and/or writing.
void fs_ramfs_file_open(
    badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, vfs_file_obj_t *file, char const *name, size_t name_len
);
// Close a file opened by `fs_ramfs_file_open`.
// Only raises an error if `file` is an invalid file descriptor.
void fs_ramfs_file_close(vfs_t *vfs, vfs_file_obj_t *file);
// Read bytes from a file.
void fs_ramfs_file_read(
    badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *file, fileoff_t offset, uint8_t *readbuf, fileoff_t readlen
);
// Write bytes from a file.
void fs_ramfs_file_write(
    badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *file, fileoff_t offset, uint8_t const *writebuf, fileoff_t writelen
);
// Change the length of a file opened by `fs_ramfs_file_open`.
void fs_ramfs_file_resize(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *file, fileoff_t new_size);

// Commit all pending writes to disk.
// The filesystem, if it does caching, must always sync everything to disk at once.
void fs_ramfs_flush(badge_err_t *ec, vfs_t *vfs);
