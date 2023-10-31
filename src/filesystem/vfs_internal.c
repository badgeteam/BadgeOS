
// SPDX-License-Identifier: MIT

#include "filesystem/vfs_internal.h"

#include "assertions.h"
#include "badge_strings.h"
#include "filesystem/vfs_ramfs.h"
#include "log.h"
#include "malloc.h"

#include <stdatomic.h>

// Index in the VFS table of the filesystem mounted at /.
// Set to -1 if no filesystem is mounted at /.
// If no filesystem is mounted at /, the FS API will not work.
ptrdiff_t vfs_root_index                  = -1;
// Table of mounted filesystems.
vfs_t     vfs_table[FILESYSTEM_MOUNT_MAX] = {0};
// Mutex for filesystem mounting / unmounting.
// Taken exclusively during mount / unmount operations.
// Taken shared during filesystem access.
mutex_t   vfs_mount_mtx                   = MUTEX_T_INIT_SHARED;
// Mutex for creating and destroying directory and file handles.
// Taken exclusively when a handle is created or destroyed.
// Taken shared when a handle is used.
mutex_t   vfs_handle_mtx                  = MUTEX_T_INIT_SHARED;

// List of open shared file handles.
vfs_file_shared_t **vfs_file_shared_list;
// Number of open shared file handles.
size_t              vfs_file_shared_list_len;
// Capacity of open shared file handles list.
size_t              vfs_file_shared_list_cap;

// List of open file handles.
vfs_file_handle_t *vfs_file_handle_list;
// Number of open file handles.
size_t             vfs_file_handle_list_len;
// Capacity of open file handles list.
size_t             vfs_file_handle_list_cap;

// Next file / directory handle number.
static atomic_int vfs_handle_no = 0;

// Abstraction for return thing from VFS.
#define vfs_impl_return(type, method, ...)                                                                             \
    do {                                                                                                               \
        switch (type) {                                                                                                \
            case FS_TYPE_RAMFS: return vfs_ramfs_##method(__VA_ARGS__);                                                \
            default: __builtin_unreachable();                                                                          \
        }                                                                                                              \
    } while (0)

// Abstraction for call function from VFS.
#define vfs_impl_call(type, rettype, method, ...)                                                                      \
    ({                                                                                                                 \
        rettype vfs_impl_call_rv;                                                                                      \
        switch (type) {                                                                                                \
            case FS_TYPE_RAMFS: vfs_impl_call_rv = vfs_ramfs_##method(__VA_ARGS__); break;                             \
            default: __builtin_unreachable();                                                                          \
        }                                                                                                              \
        vfs_impl_call_rv;                                                                                              \
    })

// Abstraction for call function returning void from VFS.
#define vfs_impl_call_void(type, method, ...)                                                                          \
    do {                                                                                                               \
        switch (type) {                                                                                                \
            case FS_TYPE_RAMFS: vfs_ramfs_##method(__VA_ARGS__); break;                                                \
            default: __builtin_unreachable();                                                                          \
        }                                                                                                              \
    } while (0)



// Create a new file_t number.
static file_t next_fileno() {
    return atomic_fetch_add(&vfs_handle_no, 1);
}



/* ==== Thread-unsafe functions ==== */

// Look up shared file handle by pointer.
static ptrdiff_t vfs_file_by_ptr(vfs_file_shared_t *ptr) {
    for (size_t i = 0; i < vfs_file_shared_list_len; i++) {
        if (vfs_file_shared_list[i] == ptr) {
            return i;
        }
    }
    return -1;
}

// Splice a shared file handle out of the list.
static void vfs_file_shared_splice(ptrdiff_t i) {
    // Remove an entry.
    vfs_file_shared_list_len--;
    if (vfs_file_shared_list_len) {
        vfs_file_shared_list[i] = vfs_file_shared_list[vfs_file_shared_list_len];
    }

    if (vfs_file_shared_list_cap > vfs_file_shared_list_len * 2) {
        // Shrink the array.
        size_t new_cap = vfs_file_shared_list_cap / 2;
        if (new_cap < 2)
            new_cap = 2;
        void *mem = realloc(vfs_file_shared_list, sizeof(*vfs_file_shared_list) * new_cap);
        if (!mem)
            return;
        vfs_file_shared_list     = mem;
        vfs_file_shared_list_cap = new_cap;
    }
}

// Splice a file handle out of the list.
static void vfs_file_handle_splice(ptrdiff_t i) {
    // Remove an entry.
    vfs_file_handle_list_len--;
    if (vfs_file_handle_list_len) {
        vfs_file_handle_list[i] = vfs_file_handle_list[vfs_file_handle_list_len];
    }

    if (vfs_file_handle_list_cap > vfs_file_handle_list_len * 2) {
        // Shrink the array.
        size_t new_cap = vfs_file_handle_list_cap / 2;
        if (new_cap < 2)
            new_cap = 2;
        void *mem = realloc(vfs_file_handle_list, sizeof(*vfs_file_handle_list) * new_cap);
        if (!mem)
            return;
        vfs_file_handle_list     = mem;
        vfs_file_handle_list_cap = new_cap;
    }
}

// Find a shared file handle by inode, if any.
ptrdiff_t vfs_shared_by_inode(vfs_t *vfs, inode_t inode) {
    for (size_t i = 0; i < vfs_file_shared_list_len; i++) {
        if (vfs_file_shared_list[i]->vfs == vfs && vfs_file_shared_list[i]->inode == inode) {
            return i;
        }
    }
    return -1;
}

// Get a file handle by number.
ptrdiff_t vfs_file_by_handle(file_t fileno) {
    for (size_t i = 0; i < vfs_file_handle_list_len; i++) {
        if (vfs_file_handle_list[i].fileno == fileno) {
            return i;
        }
    }
    return -1;
}

// Create a new empty shared file handle.
ptrdiff_t vfs_file_create_shared() {
    if (vfs_file_shared_list_len >= vfs_file_shared_list_cap) {
        // Expand list.
        size_t new_cap = vfs_file_shared_list_cap * 2;
        if (new_cap < 2)
            new_cap = 2;
        void *mem = realloc(vfs_file_shared_list, sizeof(*vfs_file_shared_list) * new_cap);
        if (!mem)
            return -1;
        vfs_file_shared_list     = mem;
        vfs_file_shared_list_cap = new_cap;
    }

    // Allocate new shared handle.
    ptrdiff_t          shared = vfs_file_shared_list_len;
    vfs_file_shared_t *shptr  = malloc(sizeof(vfs_file_shared_t));
    if (!shptr)
        return -1;
    *shptr = (vfs_file_shared_t){
        .refcount = 0,
        .index    = shared,
        .size     = 0,
        .inode    = 0,
        .vfs      = NULL,
    };
    vfs_file_shared_list[vfs_file_shared_list_len] = shptr;
    vfs_file_shared_list_len++;

    return shared;
}

// Create a new file handle.
// If `shared` is -1, a new empty shared handle is created.
ptrdiff_t vfs_file_create_handle(ptrdiff_t shared) {
    if (shared == -1) {
        // Allocate new shared handle.
        shared = vfs_file_create_shared();
    }
    if (shared < 0) {
        // Failed to allocate or illegal argument.
        return -1;
    }

    if (vfs_file_handle_list_len >= vfs_file_handle_list_cap) {
        // Expand list.
        size_t new_cap = vfs_file_handle_list_cap * 2;
        if (new_cap < 2)
            new_cap = 2;
        void *mem = realloc(vfs_file_handle_list, sizeof(*vfs_file_handle_list) * new_cap);
        if (!mem)
            return -1;
        vfs_file_handle_list     = mem;
        vfs_file_handle_list_cap = new_cap;
    }

    // Allocate new handle.
    ptrdiff_t handle = vfs_file_handle_list_len;
    vfs_file_handle_list_len++;
    vfs_file_handle_list[handle] = (vfs_file_handle_t){
        .offset = 0,
        .shared = vfs_file_shared_list[shared],
        .fileno = next_fileno(),
        .mutex  = MUTEX_T_INIT,
    };

    return handle;
}

// Destroy a shared file handle assuming the underlying file is already closed.
void vfs_file_destroy_shared(ptrdiff_t shared) {
    vfs_file_shared_splice(shared);
}

// Delete a file handle.
// If this is the last handle referring to one file, the shared handle is closed too.
void vfs_file_destroy_handle(ptrdiff_t handle) {
    assert_dev_drop(handle >= 0 && handle < (ptrdiff_t)vfs_file_handle_list_len);

    // Drop refcount.
    vfs_file_handle_list[handle].shared->refcount--;
    if (vfs_file_handle_list[handle].shared->refcount == 0) {
        // Close shared handle.
        vfs_file_close(NULL, vfs_file_handle_list[handle].shared);
        ptrdiff_t shared = vfs_file_by_ptr(vfs_file_handle_list[handle].shared);
        vfs_file_shared_splice(shared);
    }

    // Splice the handle out of the list.
    vfs_file_handle_splice(handle);
}



/* ==== Thread-safe functions ==== */

// Open the root directory of the root filesystem.
void vfs_root_open(badge_err_t *ec, vfs_file_shared_t *dir) {
    vfs_t *vfs = &vfs_table[vfs_root_index];
    vfs_impl_call_void(vfs->type, root_open, ec, vfs, dir);
}



// Insert a new file into the given directory.
// If the file already exists, does nothing.
// If `open` is true, a new handle to the file is opened.
void vfs_create_file(badge_err_t *ec, vfs_file_shared_t *dir, char const *name) {
    vfs_impl_call_void(dir->vfs->type, create_file, ec, dir->vfs, dir, name);
}

// Insert a new directory into the given directory.
// If the file already exists, does nothing.
// If `open` is true, a new handle to the directory is opened.
void vfs_create_dir(badge_err_t *ec, vfs_file_shared_t *dir, char const *name) {
    vfs_impl_call_void(dir->vfs->type, create_dir, ec, dir->vfs, dir, name);
}

// Unlink a file from the given directory.
// If this is the last reference to an inode, the inode is deleted.
void vfs_unlink(badge_err_t *ec, vfs_file_shared_t *dir, char const *name) {
    vfs_impl_call_void(dir->vfs->type, unlink, ec, dir->vfs, dir, name);
}



// Atomically read all directory entries and cache them into the directory handle.
// Refer to `dirent_t` for the structure of the cache.
void vfs_dir_read(badge_err_t *ec, vfs_file_handle_t *dir) {
    vfs_impl_call_void(dir->shared->vfs->type, dir_read, ec, dir->shared->vfs, dir);
}

// Atomically read the directory entry with the matching name.
// Returns true if the entry was found.
bool vfs_dir_find_ent(badge_err_t *ec, vfs_file_shared_t *dir, dirent_t *ent, char const *name) {
    vfs_impl_return(dir->vfs->type, dir_find_ent, ec, dir->vfs, dir, ent, name);
}



// Open a file for reading and/or writing given parent directory handle.
// Also handles OFLAGS_EXCLUSIVE and OFLAGS_CREATE.
void vfs_file_open(
    badge_err_t *ec, vfs_file_shared_t *dir, vfs_file_shared_t *file, char const *name, oflags_t oflags
) {
    vfs_t *vfs = dir->vfs;

    // Handle opening flags related to file creation.
    if (oflags & (OFLAGS_EXCLUSIVE | OFLAGS_CREATE)) {
        // Test for file existence.
        bool exists = vfs_impl_call(vfs->type, bool, exists, ec, vfs, dir, name);

        if ((oflags & OFLAGS_EXCLUSIVE) && exists) {
            badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_EXISTS);
            return;

        } else if (!(oflags & OFLAGS_CREATE) && !exists) {
            badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOTFOUND);
            return;

        } else if (oflags & OFLAGS_DIRECTORY) {
            // Create directory as requested.
            vfs_impl_call_void(vfs->type, create_dir, ec, vfs, dir, name);

        } else {
            // Create file as requested.
            vfs_impl_call_void(vfs->type, create_file, ec, vfs, dir, name);
        }
    }

    // Open the file in question.
    vfs_impl_call_void(vfs->type, file_open, ec, vfs, dir, file, name);
}

// Close a file opened by `vfs_file_open`.
// Only raises an error if `file` is an invalid file descriptor.
void vfs_file_close(badge_err_t *ec, vfs_file_shared_t *file) {
    vfs_t *vfs = file->vfs;
    vfs_impl_call_void(vfs->type, file_close, ec, vfs, file);
}

// Read bytes from a file.
void vfs_file_read(badge_err_t *ec, vfs_file_shared_t *file, fileoff_t offset, uint8_t *readbuf, fileoff_t readlen) {
    vfs_impl_call_void(file->vfs->type, file_read, ec, file->vfs, file, offset, readbuf, readlen);
}

// Write bytes to a file.
void vfs_file_write(
    badge_err_t *ec, vfs_file_shared_t *file, fileoff_t offset, uint8_t const *writebuf, fileoff_t writelen
) {
    vfs_impl_call_void(file->vfs->type, file_write, ec, file->vfs, file, offset, writebuf, writelen);
}

// Change the length of a file opened by `vfs_file_open`.
void vfs_file_resize(badge_err_t *ec, vfs_file_shared_t *file, fileoff_t new_size) {
    vfs_impl_call_void(file->vfs->type, file_resize, ec, file->vfs, file, new_size);
}



// Commit all pending writes to disk.
// The filesystem, if it does caching, must always sync everything to disk at once.
void vfs_flush(badge_err_t *ec, vfs_t *vfs) {
    vfs_impl_call_void(vfs->type, flush, ec, vfs);
}
