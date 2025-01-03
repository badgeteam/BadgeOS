
// SPDX-License-Identifier: MIT

#pragma once

#include "filesystem/vfs_internal.h"

// Try to mount a FAT filesystem.
void vfs_fat_mount(badge_err_t *ec, vfs_t *vfs);
// Unmount a FAT filesystem.
void vfs_fat_umount(vfs_t *vfs);
// Identify whether a block device contains a FAT filesystem.
// Returns false on error.
bool vfs_fat_detect(badge_err_t *ec, blkdev_t *dev);

// Atomically read all directory entries and cache them into the directory handle.
// Refer to `dirent_t` for the structure of the cache.
void vfs_fat_dir_read(badge_err_t *ec, vfs_file_desc_t *dir);
// Open a file for reading and/or writing.
void vfs_fat_file_open(badge_err_t *ec, vfs_file_obj_t *file, char const *path, oflags_t oflags);
// Clone a file opened by `vfs_fat_file_open`.
// Only raises an error if `file` is an invalid file descriptor.
void vfs_fat_file_close(badge_err_t *ec, vfs_file_obj_t *file);
// Read bytes from a file.
void vfs_fat_file_read(badge_err_t *ec, vfs_file_obj_t *file, fileoff_t offset, uint8_t *readbuf, fileoff_t readlen);
// Write bytes from a file.
void vfs_fat_file_write(
    badge_err_t *ec, vfs_file_obj_t *file, fileoff_t offset, uint8_t const *writebuf, fileoff_t writelen
);
// Change the length of a file opened by `vfs_fat_file_open`.
void vfs_fat_file_resize(badge_err_t *ec, vfs_file_obj_t *file, fileoff_t new_size);

// Commit all pending writes to disk.
// The filesystem, if it does caching, must always sync everything to disk at once.
void vfs_fat_flush(badge_err_t *ec, vfs_fat_t *vfs);
