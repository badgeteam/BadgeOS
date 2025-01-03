
// SPDX-License-Identifier: MIT

#pragma once

#include "filesystem.h"
#include "filesystem/vfs_types.h"
#include "mutex.h"



// Open the root directory of the root filesystem.
vfs_file_obj_t *vfs_root_open();

// Insert a new file into the given directory.
// If the file already exists, does nothing.
void vfs_create_file(badge_err_t *ec, vfs_file_obj_t *dir, char const *name, size_t name_len);
// Insert a new directory into the given directory.
// If the file already exists, does nothing.
void vfs_create_dir(badge_err_t *ec, vfs_file_obj_t *dir, char const *name, size_t name_len);
// Unlink a file from the given directory.
// If this is the last reference to an inode, the inode is deleted.
void vfs_unlink(badge_err_t *ec, vfs_file_obj_t *dir, char const *name, size_t name_len);

// Read all entries from a directory.
dirent_list_t vfs_dir_read(badge_err_t *ec, vfs_file_obj_t *dir);
// Read the directory entry with the matching name.
// Returns true if the entry was found.
bool          vfs_dir_find_ent(badge_err_t *ec, vfs_file_obj_t *dir, dirent_t *ent, char const *name, size_t name_len);

// Open a file or directory for reading and/or writing given parent directory handle.
vfs_file_obj_t *vfs_file_open(badge_err_t *ec, vfs_file_obj_t *dir, char const *name, size_t name_len);
// Create a new empty file.
vfs_file_obj_t *vfs_file_create(badge_err_t *ec, vfs_file_obj_t *dir, char const *name, size_t name_len);
// Duplicate a file or directory handle.
vfs_file_obj_t *vfs_file_dup(vfs_file_obj_t *orig);
// Close a file opened by `vfs_file_open`.
// Only raises an error if `file` is an invalid file descriptor.
void            vfs_file_close(vfs_file_obj_t *file);
// Read bytes from a file.
// The entire read succeeds or the entire read fails, never partial read.
void vfs_file_read(badge_err_t *ec, vfs_file_obj_t *file, fileoff_t offset, uint8_t *readbuf, fileoff_t readlen);
// Write bytes to a file.
// If the file is not large enough, it fails.
// The entire write succeeds or the entire write fails, never partial write.
void vfs_file_write(
    badge_err_t *ec, vfs_file_obj_t *file, fileoff_t offset, uint8_t const *writebuf, fileoff_t writelen
);
// Change the length of a file opened by `vfs_file_open`.
void vfs_file_resize(badge_err_t *ec, vfs_file_obj_t *file, fileoff_t new_size);

// Commit all pending writes on a file to disk.
// The filesystem, if it does caching, must always sync everything to disk at once.
void vfs_file_flush(badge_err_t *ec, vfs_file_obj_t *file);
// Commit all pending writes to disk.
// The filesystem, if it does caching, must always sync everything to disk at once.
void vfs_flush(badge_err_t *ec, vfs_t *vfs);
