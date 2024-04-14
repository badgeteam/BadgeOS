
// SPDX-License-Identifier: MIT

#include "badge_strings.h"
#include "filesystem/vfs_internal.h"
#include "filesystem/vfs_ramfs.h"
#include "log.h"
#include "malloc.h"



// Replace a handle with the root directory of the root filesystem.
// If this method fails, the old value is preserved.
static void root_reopen(badge_err_t *ec, vfs_file_handle_t *dir) {
    badge_err_t ec0;
    if (!ec)
        ec = &ec0;
    assert_always(mutex_acquire(NULL, &vfs_handle_mtx, VFS_MUTEX_TIMEOUT));

    // Create or obtain a new shared handle.
    ptrdiff_t          shared = vfs_shared_by_inode(&vfs_table[vfs_root_index], vfs_table[vfs_root_index].inode_root);
    vfs_file_shared_t *shptr;
    if (shared == -1) {
        // Open new handle.
        shared = vfs_file_create_shared();
        if (shared == -1) {
            mutex_release(NULL, &vfs_handle_mtx);
            badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOMEM);
            return;
        }
        shptr = vfs_file_shared_list[shared];

        // Open root directory.
        vfs_root_open(ec, shptr);
        if (!badge_err_is_ok(ec)) {
            vfs_file_destroy_shared(shared);
            mutex_release(NULL, &vfs_handle_mtx);
            return;
        }
        shptr->refcount = 0;

    } else {
        // Use existing handle.
        shptr = vfs_file_shared_list[shared];
    }

    // Switch to new handle.
    vfs_file_shared_t *old = dir->shared;
    dir->shared            = shptr;
    shptr->refcount++;

    // Clean up old handle.
    old->refcount--;
    if (old->refcount == 0) {
        vfs_file_close(ec, old);
        badge_err_assert_dev(ec);
        vfs_file_destroy_shared(old->index);
    } else {
        badge_err_set_ok(ec);
    }

    mutex_release(NULL, &vfs_handle_mtx);
}

// Replace a directory handle with the handle of one of it's entries.
// If this method fails, the old value is preserved.
static void dir_reopen(badge_err_t *ec, vfs_file_handle_t *dir, dirent_t const *ent) {
    badge_err_t ec0;
    if (!ec)
        ec = &ec0;
    assert_always(mutex_acquire(NULL, &vfs_handle_mtx, VFS_MUTEX_TIMEOUT));

    // TODO: This is the location in which mounted filesystems are handled.
    // Create or obtain a new shared handle.
    ptrdiff_t          shared = vfs_shared_by_inode(dir->shared->vfs, ent->inode);
    vfs_file_shared_t *shptr;
    if (shared == -1) {
        // Open new handle.
        shared = vfs_file_create_shared();
        if (shared == -1) {
            mutex_release(NULL, &vfs_handle_mtx);
            badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOMEM);
            return;
        }
        shptr = vfs_file_shared_list[shared];

        // Open entry.
        vfs_file_open(ec, dir->shared, shptr, ent->name, 0);
        if (!badge_err_is_ok(ec)) {
            vfs_file_destroy_shared(shared);
            mutex_release(NULL, &vfs_handle_mtx);
            return;
        }
        shptr->refcount = 0;

    } else {
        // Use existing handle.
        shptr = vfs_file_shared_list[shared];
    }

    // Switch to new handle.
    vfs_file_shared_t *old = dir->shared;
    dir->shared            = shptr;
    shptr->refcount++;

    // Clean up old handle.
    old->refcount--;
    if (old->refcount == 0) {
        vfs_file_close(ec, old);
        badge_err_assert_dev(ec);
        vfs_file_destroy_shared(old->index);
    } else {
        badge_err_set_ok(ec);
    }

    mutex_release(NULL, &vfs_handle_mtx);
}

// Walk the filesystem and locate a path relative to `dir`.
// If the first character is '/', `dir` is first replaced by the root directory.
//
// If `follow_symlink` is true, all symlinks are resolved.
// Otherwise, if the last element in the absolute path is a symlink, it is not resolved.
//
// May use the first `FILESYSTEM_PATH_MAX` characters of `path` as a path buffer.
// Regardless of whether the file is found, the real filename will be the segment after the last '/'.
//
// Replaces the value of `dir` to be the shared directory handle of the parent directory of the subject file.
// If the subject file is a directory, it may also be named "." or ".." in which the parent directory will be itself or
// a subdirectory respectively. The root directory of the root filesystem may also be it's own parent for "..".
//
// If the file exists, reads the directory entry into `ent`.
// If the file does not exist, sets `inode` to 0.
//
// Returns the index in the path buffer of the filename after symlinks if the parent directory exists,
// -2 if the directory is definitely the root directory, or -1 otherwise.
static ptrdiff_t walk(badge_err_t *ec, vfs_file_handle_t *dir, char path[FILESYSTEM_PATH_MAX + 1], dirent_t *ent) {
    assert_dev_drop(dir != NULL);
    badge_err_t ec0;
    if (!ec)
        ec = &ec0;

    // First character of the next filename.
    ptrdiff_t begin = 0;
    // One pas last character of the next filename.
    ptrdiff_t end   = 0;
    // Current length of the path string.
    ptrdiff_t len   = (ptrdiff_t)cstr_length(path);
    // Whether the file was found.
    bool      found = false;

    // Deduplicate forward slashes.
    for (ptrdiff_t i = 0; i < len;) {
        if (path[i] == '/' && path[i + 1] == '/') {
            cstr_copy(path + i + 1, FILESYSTEM_PATH_MAX + 1, path + i + 2);
            len--;
        } else {
            i++;
        }
    }

    // Absolute paths starting with '/'.
    if (path[0] == '/') {
        root_reopen(ec, dir);
        if (!badge_err_is_ok(ec)) {
            ent->inode = 0;
            return -1;
        }
    }
    if (path[0] == '/' && len == 1) {
        return -2;
    }

    // Locate the relative file on disk.
    while (begin < len) {
        // Test for directory.
        if (path[begin] == '/') {
            if (!dir->is_dir) {
                badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_IS_FILE);
                ent->inode = 0;
                return -1;
            }
            begin++;
            continue;
        }

        // Find the next delimiter.
        end = cstr_index_from(path, '/', begin);
        if (end < 0)
            end = len;

        // Read current directory.
        char tmp    = path[end];
        path[end]   = 0;
        found       = vfs_dir_find_ent(ec, dir->shared, ent, path + begin);
        path[end]   = tmp;
        // Whether the current entry represents an intermediate directory.
        bool is_int = path[end] == '/' && path[end + 1] != 0;

        if (!found && is_int) {
            // An intermediate directory was not found.
            badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOTFOUND);
            begin = -1;
            break;

        } else if (!is_int) {
            // The final file or directory.
            badge_err_set_ok(ec);
            if (!found)
                ent->inode = 0;
            break;

        } else /* found && is_int */ {
            // An intermediate directory to open.
            dir_reopen(ec, dir, ent);
            if (!badge_err_is_ok(ec)) {
                begin = -1;
                break;
            }
            begin = end;
        }
    }

    // Remove trailing forward slashes.
    while (len > 1 && path[len - 1] == '/') {
        path[len - 1] = 0;
        len--;
    }

    return begin;
}

// Open a new file handle to the root directory.
static vfs_file_handle_t *root_open(badge_err_t *ec) {
    badge_err_t ec0;
    if (!ec)
        ec = &ec0;
    assert_always(mutex_acquire(NULL, &vfs_handle_mtx, VFS_MUTEX_TIMEOUT));

    ptrdiff_t          existing = vfs_shared_by_inode(&vfs_table[vfs_root_index], vfs_table[vfs_root_index].inode_root);
    ptrdiff_t          handle   = vfs_file_create_handle(existing);
    vfs_file_handle_t *ptr      = &vfs_file_handle_list[handle];

    if (existing == -1) {
        // Open new shared handle.
        vfs_root_open(ec, ptr->shared);
        if (!badge_err_is_ok(ec)) {
            vfs_file_destroy_handle(handle);
            mutex_release(NULL, &vfs_handle_mtx);
            return NULL;
        }
    }

    // Create file handle.
    ptr->offset         = 0;
    ptr->write          = false;
    ptr->read           = true;
    ptr->is_dir         = true;
    ptr->dir_cache      = NULL;
    ptr->dir_cache_size = 0;

    mutex_release(NULL, &vfs_handle_mtx);
    return ptr;
}



// Check the validity of a mount point.
// Creates a copy of the mount point if it is valid.
static char *check_mountpoint(badge_err_t *ec, char const *raw) {
    // Mount points are absolute paths.
    if (raw[0] != '/') {
        logkf(LOG_ERROR, "check_mountpoint: %{cs}: Mount point is relative path", raw);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        return NULL;
    }

    // Check path length.
    size_t raw_len = cstr_length(raw);
    if (raw_len > FILESYSTEM_PATH_MAX) {
        logkf(LOG_ERROR, "check_mountpoint: %{cs}: Mount path too long");
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        return NULL;
    }

    // Assert that the path is a canonical path.
    if (!fs_is_canonical_path(raw)) {
        logkf(LOG_ERROR, "check_mountpoint: %{cs}: Mount point is not a canonical path", raw);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        return NULL;
    }

    // Create a copy of the path.
    char *copy = malloc(raw_len + 1);
    if (!copy) {
        logkf(LOG_ERROR, "check_mountpoint: Out of memory (allocating %{size;d} bytes)", raw_len + 1);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOMEM);
        return NULL;
    }
    mem_copy(copy, raw, raw_len + 1);
    if (raw_len > 1 && copy[raw_len] == '/') {
        copy[raw_len] = 0;
    }

    // Assert that the path is not in use.
    for (size_t i = 0; i < FILESYSTEM_MOUNT_MAX; i++) {
        if (vfs_table[i].mountpoint && cstr_equals(vfs_table[i].mountpoint, copy)) {
            logkf(LOG_ERROR, "check_mountpoint: %{cs}: Mount point is in use", copy);
            badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_INUSE);
            free(copy);
            return NULL;
        } else if (vfs_table[i].mountpoint && cstr_equals_case(vfs_table[i].mountpoint, copy)) {
            logkf(LOG_WARN, "check_mountpoint: %{cs}: Very similar to %{cs}", copy, vfs_table[i].mountpoint);
        }
    }

    // If not root filesystem, assert directory exists.
    if (!cstr_equals(copy, "/")) {
        file_t dir = fs_dir_open(ec, copy, 0);
        if (ec->cause == ECAUSE_NOTFOUND) {
            logkf(LOG_ERROR, "check_mountpoint: %{cs}: Mount point does not exist");
        } else if (ec->cause == ECAUSE_IS_FILE) {
            logkf(LOG_ERROR, "check_mountpoint: %{cs}: Mount point is not a directory");
        }
        if (dir == FILE_NONE) {
            free(copy);
            return NULL;
        }
        fs_dir_close(NULL, dir);
    }

    // If all that passes, the mount point is valid.
    return copy;
}

// Try to mount a filesystem.
// Some filesystems (like RAMFS) do not use a block device, for which `media` must be NULL.
// Filesystems which do use a block device can often be automatically detected.
void fs_mount(badge_err_t *ec, fs_type_t type, blkdev_t *media, char const *mountpoint, mountflags_t flags) {
    badge_err_t ec0;
    if (!ec)
        ec = &ec0;

    // Take the filesystem mounting mutex.
    assert_always(mutex_acquire(NULL, &vfs_mount_mtx, VFS_MUTEX_TIMEOUT));

    if (type == FS_TYPE_UNKNOWN && !media) {
        // Block device is required to auto-detect.
        logk(LOG_ERROR, "fs_mount: Neither media nor type specified.");
        mutex_release(NULL, &vfs_mount_mtx);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
    } else if (type == FS_TYPE_UNKNOWN) {
        // Try to auto-detect filesystem.
        type = fs_detect(ec, media);
        if (!badge_err_is_ok(ec))
            return;
        if (type == FS_TYPE_UNKNOWN) {
            logk(LOG_ERROR, "fs_mount: Unable to determine filesystem type.");
            mutex_release(NULL, &vfs_mount_mtx);
            badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_UNAVAIL);
            return;
        }
    }

    // Check the validity of the mount point.
    char *mountpoint_copy = check_mountpoint(ec, mountpoint);
    if (!badge_err_is_ok(ec)) {
        mutex_release(NULL, &vfs_mount_mtx);
        return;
    }

    // Check for space in VFS table.
    size_t vfs_index;
    for (vfs_index = 0; vfs_index < FILESYSTEM_MOUNT_MAX; vfs_index++) {
        if (!vfs_table[vfs_index].mountpoint) {
            break;
        }
    }
    if (vfs_index == FILESYSTEM_MOUNT_MAX) {
        logk(LOG_ERROR, "fs_mount: Mounted filesystem limit (" comptime_stringify(FILESYSTEM_MOUNT_MAX) ") reached.");
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_UNAVAIL);
        free(mountpoint_copy);
        mutex_release(NULL, &vfs_mount_mtx);
        return;
    }

    // Check writeability.
    if (!(flags & MOUNTFLAGS_READONLY) && media && media->readonly) {
        logk(LOG_ERROR, "fs_mount: Writeable filesystem on readonly media.");
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_READONLY);
        goto error_cleanup;
    }

    // Fill out the VFS entry.
    vfs_table[vfs_index] = (vfs_t){
        .mountpoint = mountpoint_copy,
        .readonly   = flags & MOUNTFLAGS_READONLY,
        .media      = media,
        .type       = type,
    };

    // Delegate to filesystem-specific mount.
    switch (type) {
        case FS_TYPE_FAT: vfs_fat_mount(ec, &vfs_table[vfs_index]); break;
        case FS_TYPE_RAMFS: vfs_ramfs_mount(ec, &vfs_table[vfs_index]); break;
        default: badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM); break;
    }
    if (!badge_err_is_ok(ec)) {
        logk(LOG_ERROR, "fs_mount: Mount error reported by VFS.");
        goto error_cleanup;
    }

    if (cstr_equals(mountpoint, "/")) {
        // Set root mountpoint index.
        vfs_root_index = (ptrdiff_t)vfs_index;
    }

    // At this point, the filesystem is ready for use.
    mutex_release(NULL, &vfs_mount_mtx);
    return;

error_cleanup:
    free(vfs_table[vfs_index].mountpoint);
    vfs_table[vfs_index].mountpoint = NULL;
    mutex_release(NULL, &vfs_mount_mtx);
}

// Unmount a filesystem.
// Only raises an error if there isn't a valid filesystem to unmount.
void fs_umount(badge_err_t *ec, char const *mountpoint) {
    // TODO: Remove redundant slashes from path.
    size_t vfs_index;
    for (vfs_index = 0; vfs_index < FILESYSTEM_MOUNT_MAX; vfs_index++) {
        if (cstr_equals(vfs_table[vfs_index].mountpoint, mountpoint)) {
            break;
        }
    }
    if (vfs_index == FILESYSTEM_MOUNT_MAX) {
        logkf(LOG_ERROR, "fs_umount: %{cs}: Not mounted.", mountpoint);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOTFOUND);
        return;
    }

    // Close file handles.
    for (size_t i = 0; i < vfs_file_handle_list_len; i++) {
        if (vfs_file_handle_list[i].shared->vfs == &vfs_table[vfs_index]) {
            fs_close(NULL, vfs_file_handle_list[i].fileno);
        }
    }

    // Delegate to filesystem-specific mount.
    switch (vfs_table[vfs_index].type) {
        case FS_TYPE_FAT: vfs_fat_umount(&vfs_table[vfs_index]); break;
        case FS_TYPE_RAMFS: vfs_ramfs_umount(&vfs_table[vfs_index]); break;
        default: __builtin_unreachable();
    }

    // Release memory.
    free(vfs_table[vfs_index].mountpoint);
    vfs_table[vfs_index].mountpoint = NULL;
}

// Try to identify the filesystem stored in the block device
// Returns `FS_TYPE_UNKNOWN` on error or if the filesystem is unknown.
fs_type_t fs_detect(badge_err_t *ec, blkdev_t *media) {
    (void)media;
    if (vfs_fat_detect(ec, media)) {
         return FS_TYPE_FAT;
    } else {
    badge_err_set_ok(ec);
    return FS_TYPE_UNKNOWN;
    }
}



// Test whether a path is a canonical path, but not for the existence of the file or directory.
// A canonical path starts with '/' and contains none of the following regex: `\.\.?/|/\.\.?$|//+`
bool fs_is_canonical_path(char const *path) {
    if (*path != '/') {
        return false;
    }
    while (*path) {
        if (path[0] == '.') {
            size_t i = path[1] == '.';
            if (path[i] == 0 || path[i] == '/') {
                return false;
            }
        } else if (path[0] == '/' && path[1] == '/') {
            return false;
        }
        ptrdiff_t index = cstr_index(path, '/');
        if (index == -1) {
            return true;
        }
        path = path + index + 1;
    }
    return true;
}



// Test that the handle exists and is a directory handle.
static bool is_dir_handle(badge_err_t *ec, file_t dir) {
    assert_always(mutex_acquire_shared(ec, &vfs_handle_mtx, VFS_MUTEX_TIMEOUT));

    // Check the handle exists.
    ptrdiff_t index = vfs_file_by_handle(dir);
    if (index < 0) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        mutex_release_shared(ec, &vfs_handle_mtx);
        return false;
    }
    // Check the handle is that of a directory.
    vfs_file_handle_t *handle = &vfs_file_handle_list[index];
    if (!handle->is_dir) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_IS_FILE);
        mutex_release_shared(ec, &vfs_handle_mtx);
        return false;
    }

    mutex_release_shared(ec, &vfs_handle_mtx);
    return true;
}

// Create a new directory.
// Returns whether the target exists and is a directory.
bool fs_dir_create(badge_err_t *ec, char const *path) {
    file_t fd = fs_open(ec, path, OFLAGS_DIRECTORY | OFLAGS_READONLY | OFLAGS_CREATE);
    if (fd >= 0) {
        fs_close(ec, fd);
    } else if (ec && badge_err_is_ok(ec)) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_IS_FILE);
    }
    return fd >= 0;
}

// Open a directory for reading.
file_t fs_dir_open(badge_err_t *ec, char const *path, oflags_t oflags) {
    return fs_open(ec, path, oflags | OFLAGS_DIRECTORY | OFLAGS_READONLY);
}

// Close a directory opened by `fs_dir_open`.
// Only raises an error if `dir` is an invalid directory descriptor.
void fs_dir_close(badge_err_t *ec, file_t dir) {
    if (!is_dir_handle(ec, dir))
        return;
    fs_close(ec, dir);
}

// Read the current directory entry.
// See also: `fs_dir_read_name`.
bool fs_dir_read(badge_err_t *ec, dirent_t *dirent_out, file_t dir) {
    if (!is_dir_handle(ec, dir))
        return false;
    badge_err_t ec0 = {.cause = ECAUSE_OK};
    if (!ec)
        ec = &ec0;

    // Read the entry but not the name.
    fileoff_t pos = fs_tell(ec, dir);
    if (!badge_err_is_ok(ec))
        return false;
    fileoff_t read_len = sizeof(dirent_t) - FILESYSTEM_NAME_MAX - 1;
    fileoff_t len      = fs_read(ec, dir, dirent_out, read_len);
    if (len == 0 && badge_err_is_ok(ec)) {
        return false;
    }
    if (!badge_err_is_ok(ec)) {
        return false;
    }
    // Bounds check read, name and record length.
    if (dirent_out->name_len > FILESYSTEM_NAME_MAX || dirent_out->record_len < read_len + dirent_out->name_len) {
        // Revert position, report error.
        fs_seek(NULL, dir, pos, SEEK_ABS);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_UNKNOWN);
        return false;
    }

    // Read the name.
    fileoff_t name_len = fs_read(ec, dir, dirent_out->name, dirent_out->name_len);
    // Bounds check read, test name validity.
    if (!badge_err_is_ok(ec) || name_len != dirent_out->name_len || mem_index(dirent_out->name, name_len, '/') != -1 ||
        mem_index(dirent_out->name, name_len, 0) != -1) {
        // Revert position, report error.
        fs_seek(NULL, dir, pos, SEEK_ABS);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_UNKNOWN);
        return false;
    }

    // Seek to next dirent.
    fs_seek(ec, dir, pos + dirent_out->record_len, SEEK_ABS);

    // Null-terminate name.
    dirent_out->name[name_len] = 0;
    return true;
}



// Open a file for reading and/or writing.
file_t fs_open(badge_err_t *ec, char const *path, oflags_t oflags) {
    badge_err_t ec0 = {.cause = ECAUSE_OK};
    if (!ec)
        ec = &ec0;

    // Test flag validity.
    if ((oflags & OFLAGS_DIRECTORY) && (oflags & ~VALID_OFLAGS_DIRECTORY)) {
        // A flag mutually exclusive with `OFLAGS_DIRECTORY` was given.
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        return FILE_NONE;
    } else if (!(oflags & OFLAGS_DIRECTORY) && (oflags & ~VALID_OFLAGS_FILE)) {
        // An invalid flag was given.
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        return FILE_NONE;
    } else if (!(oflags & OFLAGS_READWRITE)) {
        // Neither read nor write access is requested.
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        return FILE_NONE;
    }

    // Copy the path.
    char canon_path[FILESYSTEM_PATH_MAX + 1];
    if (path[cstr_copy(canon_path, sizeof(canon_path), path)] != 0) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_TOOLONG);
        return FILE_NONE;
    }

    // Locate the file.
    vfs_file_handle_t *parent = root_open(ec);
    if (!badge_err_is_ok(ec)) {
        return FILE_NONE;
    }
    dirent_t  ent   = {0};
    ptrdiff_t slash = walk(ec, parent, canon_path, &ent);
    bool      found = ent.inode;
    if (!badge_err_is_ok(ec)) {
        return FILE_NONE;
    }
    // Get the filename from canonical path.
    char *filename;
    if (slash == -2) {
        filename = ".";
    } else if (slash == -1) {
        filename = canon_path;
    } else {
        filename = canon_path + slash;
    }

    assert_always(mutex_acquire(ec, &vfs_handle_mtx, VFS_MUTEX_TIMEOUT));
    ptrdiff_t existing = -1;
    bool      is_dir;
    if (found) {
        // File exists.
        is_dir = ent.is_dir;

        // Check file type.
        if (ent.is_dir && !(oflags & OFLAGS_DIRECTORY)) {
            badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_IS_DIR);
            mutex_release(NULL, &vfs_handle_mtx);
            goto error;
        } else if (!ent.is_dir && (oflags & OFLAGS_DIRECTORY)) {
            badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_IS_FILE);
            mutex_release(NULL, &vfs_handle_mtx);
            goto error;
        }

        // Check for existing shared handles.
        existing = vfs_shared_by_inode(parent->shared->vfs, ent.inode);

    } else {
        // File does not exist.
        is_dir   = (oflags & OFLAGS_DIRECTORY);
        existing = -1;
    }

    // Create a new handle from existing shared handle.
    ptrdiff_t handle = vfs_file_create_handle(existing);
    if (handle == -1) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOMEM);
        mutex_release(NULL, &vfs_handle_mtx);
        goto error;
    }
    vfs_file_handle_t *ptr = &vfs_file_handle_list[handle];

    // Apply opening flags.
    ptr->read   = oflags & OFLAGS_READONLY;
    ptr->write  = oflags & OFLAGS_WRITEONLY;
    ptr->is_dir = is_dir;

    if (existing == -1) {
        // Create new shared file handle.
        vfs_file_shared_t *shared = ptr->shared;
        vfs_file_open(ec, parent->shared, shared, filename, oflags);
        if (!badge_err_is_ok(ec)) {
            vfs_file_destroy_handle(handle);
            mutex_release(NULL, &vfs_handle_mtx);
            goto error;
        }
        shared->refcount = 1;
    }

    // Successful opening of new handle.
    mutex_release(NULL, &vfs_handle_mtx);

    return ptr->fileno;

error:
    // Destroy directory handle.
    assert_always(mutex_acquire(NULL, &vfs_handle_mtx, VFS_MUTEX_TIMEOUT));
    ptrdiff_t index = vfs_file_by_handle(parent->fileno);
    vfs_file_destroy_handle(index);
    mutex_release(NULL, &vfs_handle_mtx);
    return FILE_NONE;
}

// Close a file opened by `fs_open`.
// Only raises an error if `file` is an invalid file descriptor.
void fs_close(badge_err_t *ec, file_t file) {
    assert_always(mutex_acquire(ec, &vfs_handle_mtx, VFS_MUTEX_TIMEOUT));

    ptrdiff_t index = vfs_file_by_handle(file);
    if (index == -1) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
    } else {
        badge_err_set_ok(ec);
        vfs_file_destroy_handle(index);
    }

    mutex_release(NULL, &vfs_handle_mtx);
}

// Read bytes from a file.
// Returns the amount of data successfully read.
fileoff_t fs_read(badge_err_t *ec, file_t file, void *readbuf, fileoff_t readlen) {
    if (readlen < 0) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        mutex_release_shared(NULL, &vfs_handle_mtx);
        return 0;
    }
    assert_always(mutex_acquire_shared(NULL, &vfs_handle_mtx, VFS_MUTEX_TIMEOUT));

    // Look up the handle.
    ptrdiff_t index = vfs_file_by_handle(file);
    if (index == -1) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        mutex_release_shared(NULL, &vfs_handle_mtx);
        return 0;
    }
    vfs_file_handle_t *ptr = &vfs_file_handle_list[index];

    // Check permission.
    if (!ptr->read) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PERM);
        mutex_release_shared(NULL, &vfs_handle_mtx);
        return 0;
    }

    // Read data from the handle.
    assert_always(mutex_acquire(NULL, &ptr->mutex, VFS_MUTEX_TIMEOUT));
    if (ptr->is_dir) {
        // Directory reads go through cache.
        if (ptr->offset == 0) {
            vfs_dir_read(ec, ptr);
        }
        if (ptr->offset != 0 || badge_err_is_ok(ec)) {
            if (readlen + ptr->offset < 0 || readlen + ptr->offset > ptr->dir_cache_size) {
                readlen = ptr->dir_cache_size - ptr->offset;
            }
            mem_copy(readbuf, ptr->dir_cache + ptr->offset, readlen);
            ptr->offset += readlen;
        }

    } else {
        // File reads go through VFS.
        if (readlen + ptr->offset < 0 || readlen + ptr->offset > ptr->shared->size) {
            readlen = ptr->shared->size - ptr->offset;
        }
        vfs_file_read(ec, ptr->shared, ptr->offset, readbuf, readlen);
        ptr->offset += readlen;
    }
    mutex_release(NULL, &ptr->mutex);

    mutex_release_shared(NULL, &vfs_handle_mtx);
    return readlen;
}

// Write bytes to a file.
// Returns the amount of data successfully written.
fileoff_t fs_write(badge_err_t *ec, file_t file, void const *writebuf, fileoff_t writelen) {
    if (writelen < 0) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        mutex_release_shared(NULL, &vfs_handle_mtx);
        return 0;
    }
    assert_always(mutex_acquire_shared(NULL, &vfs_handle_mtx, VFS_MUTEX_TIMEOUT));

    // Look up the handle.
    ptrdiff_t index = vfs_file_by_handle(file);
    if (index == -1) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        mutex_release_shared(NULL, &vfs_handle_mtx);
        return 0;
    }
    vfs_file_handle_t *ptr = &vfs_file_handle_list[index];

    // Check permission.
    if (!ptr->write) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PERM);
        mutex_release_shared(NULL, &vfs_handle_mtx);
        return 0;
    }

    // Read data from the handle.
    assert_always(mutex_acquire(NULL, &ptr->mutex, VFS_MUTEX_TIMEOUT));
    // File writes go through VFS.
    if (writelen + ptr->offset < 0) {
        // Integer overflow: Assume no space.
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOSPACE);
        mutex_release(NULL, &ptr->mutex);
        mutex_release_shared(NULL, &vfs_handle_mtx);
        return 0;
    }
    if (ptr->offset + writelen > ptr->shared->size) {
        vfs_file_resize(ec, ptr->shared, ptr->offset + writelen);
    }
    vfs_file_write(ec, ptr->shared, ptr->offset, writebuf, writelen);
    mutex_release(NULL, &ptr->mutex);

    mutex_release_shared(NULL, &vfs_handle_mtx);
    return writelen;
}

// Get the current offset in the file.
fileoff_t fs_tell(badge_err_t *ec, file_t file) {
    assert_always(mutex_acquire_shared(NULL, &vfs_handle_mtx, VFS_MUTEX_TIMEOUT));

    // Look up the handle.
    ptrdiff_t index = vfs_file_by_handle(file);
    if (index == -1) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        mutex_release_shared(NULL, &vfs_handle_mtx);
        return 0;
    }
    vfs_file_handle_t *ptr = &vfs_file_handle_list[index];

    // Get the position atomically.
    assert_always(mutex_acquire(NULL, &ptr->mutex, VFS_MUTEX_TIMEOUT));
    fileoff_t ret = ptr->offset;
    mutex_release(NULL, &ptr->mutex);

    mutex_release_shared(NULL, &vfs_handle_mtx);
    return ret;
}

// Set the current offset in the file.
// Returns the new offset in the file.
fileoff_t fs_seek(badge_err_t *ec, file_t file, fileoff_t off, fs_seek_t seekmode) {
    assert_always(mutex_acquire_shared(NULL, &vfs_handle_mtx, VFS_MUTEX_TIMEOUT));

    // Look up the handle.
    ptrdiff_t index = vfs_file_by_handle(file);
    if (index == -1) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        mutex_release_shared(NULL, &vfs_handle_mtx);
        return 0;
    }
    vfs_file_handle_t *ptr = &vfs_file_handle_list[index];

    // Update the position atomically.
    assert_always(mutex_acquire(NULL, &ptr->mutex, VFS_MUTEX_TIMEOUT));
    badge_err_set_ok(ec);
    switch (seekmode) {
        case SEEK_ABS: ptr->offset = off; break;
        case SEEK_CUR: ptr->offset += off; break;
        case SEEK_END: ptr->offset = ptr->shared->size + off; break;
        default: badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM); break;
    }
    // Clamp offset.
    if (ptr->offset < 0) {
        ptr->offset = 0;
    } else if (!ptr->is_dir && ptr->offset > ptr->shared->size) {
        ptr->offset = ptr->shared->size;
    } else if (ptr->is_dir && ptr->offset > ptr->dir_cache_size) {
        ptr->offset = ptr->dir_cache_size;
    }
    mutex_release(NULL, &ptr->mutex);

    mutex_release_shared(NULL, &vfs_handle_mtx);
    return ptr->offset;
}

// Force any write caches to be flushed for a given file.
// If the file is `FILE_NONE`, all open files are flushed.
void fs_flush(badge_err_t *ec, file_t file) {
    (void)file;
    badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_UNSUPPORTED);
}
