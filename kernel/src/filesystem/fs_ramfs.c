
// SPDX-License-Identifier: MIT

#include "filesystem/fs_ramfs.h"

#include "assertions.h"
#include "badge_strings.h"
#include "malloc.h"

#define RAMFS(vfs)    (*(fs_ramfs_t *)(vfs)->cookie)
#define RAMFILE(file) (*(fs_ramfs_file_t *)(file)->cookie)



// Try to resize an inode.
static bool resize_inode(badge_err_t *ec, vfs_t *vfs, fs_ramfs_inode_t *inode, size_t size) {
    (void)vfs;

    if (inode->cap >= 2 * size) {
        // If capacity is too large, try to save some memory.
        size_t cap = inode->cap / 2;
        void  *mem = realloc(inode->buf, inode->cap / 2);
        if (mem || cap == 0) {
            inode->cap /= 2;
            inode->buf  = mem;
        }
        inode->len = size;
        return true;

    } else if (inode->cap >= size) {
        // If capacity is large enough, don't bother resizing.
        inode->len = size;
        badge_err_set_ok(ec);
        return true;

    } else {
        // If the capacity is not large enough, increase the size.
        size_t cap = 1;
        while (cap < size) {
            cap *= 2;
        }
        void *mem = realloc(inode->buf, cap);
        if (mem) {
            inode->cap = cap;
            inode->buf = mem;
            inode->len = size;
            return true;
        } else {
            return false;
        }
    }
}

// Find an empty inode.
static ptrdiff_t find_inode(vfs_t *vfs) {
    for (size_t i = VFS_RAMFS_INODE_FIRST; i < RAMFS(vfs).inode_list_len; i++) {
        if (!RAMFS(vfs).inode_usage[i])
            return (ptrdiff_t)i;
    }
    return -1;
}

// Decrease the refcount of an inode and delete it if it reaches 0.
static void pop_inode_refcount(vfs_t *vfs, fs_ramfs_inode_t *inode) {
    inode->links--;
    if (inode->links == 0) {
        // Free inode.
        free(inode->buf);
        RAMFS(vfs).inode_usage[inode->inode] = false;
    }
}

// Insert a new directory entry.
static bool insert_dirent(badge_err_t *ec, vfs_t *vfs, fs_ramfs_inode_t *dir, fs_ramfs_dirent_t *ent) {
    // Allocate space in the directory.
    size_t pre_size = dir->len;
    if (!resize_inode(ec, vfs, dir, pre_size + ent->size)) {
        return false;
    }

    // Copy to the destination.
    mem_copy(dir->buf + pre_size, ent, ent->size);
    return true;
}

// Remove a directory entry.
// Takes a pointer to an entry in the directory's buffer.
static void remove_dirent(vfs_t *vfs, fs_ramfs_inode_t *dir, fs_ramfs_dirent_t *ent) {
    size_t off      = (size_t)ent - (size_t)dir->buf;
    size_t ent_size = ent->size;

    // Copy back entries further in.
    mem_copy(dir->buf + off, dir->buf + off + ent_size, dir->len - ent_size);

    // Resize the buffer.
    resize_inode(NULL, vfs, dir, dir->len - ent_size);
}

// Find the directory entry of a given filename in a directory.
// Returns a pointer to an entry in the directory's buffer, or NULL if not found.
static fs_ramfs_dirent_t *
    find_dirent(badge_err_t *ec, vfs_t *vfs, fs_ramfs_inode_t *dir, char const *name, size_t name_len) {
    (void)vfs;
    badge_err_set_ok(ec);
    size_t off = 0;
    while (off < dir->len) {
        fs_ramfs_dirent_t *ent = (fs_ramfs_dirent_t *)(dir->buf + off);
        if (ent->name_len == name_len && mem_equals(name, ent->name, name_len)) {
            return ent;
        }
        off += ent->size;
    }
    return NULL;
}

// Insert a new file or directory into the given directory.
// If the file already exists, does nothing.
static fs_ramfs_inode_t *
    create_file(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, char const *name, size_t name_len, filetype_t type) {
    if (name_len > VFS_RAMFS_NAME_MAX) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_TOOLONG);
    }
    assert_always(mutex_acquire(NULL, &RAMFS(vfs).mtx, TIMESTAMP_US_MAX));

    // Test whether the file already exists.
    fs_ramfs_inode_t  *dirptr   = RAMFILE(dir);
    fs_ramfs_dirent_t *existing = find_dirent(ec, vfs, dirptr, name, name_len);
    if (existing) {
        mutex_release(NULL, &RAMFS(vfs).mtx);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_EXISTS);
        return NULL;
    }

    // Find a vacant inode to assign.
    ptrdiff_t inum = find_inode(vfs);
    if (inum == -1) {
        mutex_release(NULL, &RAMFS(vfs).mtx);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOSPACE);
        return NULL;
    }

    // Write a directory entry.
    fs_ramfs_dirent_t ent = {
        .size     = offsetof(fs_ramfs_dirent_t, name) + name_len + 1,
        .name_len = name_len,
        .inode    = inum,
    };
    ent.size += (~ent.size + 1) % sizeof(size_t);
    mem_copy(ent.name, name, name_len);
    ent.name[name_len] = 0;

    // Set up inode.
    fs_ramfs_inode_t *iptr = &RAMFS(vfs).inode_list[inum];

    iptr->buf   = NULL;
    iptr->len   = 0;
    iptr->cap   = 0;
    iptr->inode = inum;
    iptr->mode  = (type << VFS_RAMFS_MODE_BIT) | 0777; /* TODO. */
    iptr->links = 1;
    iptr->uid   = 0; /* TODO. */
    iptr->gid   = 0; /* TODO. */

    // Copy into the end of the directory.
    if (insert_dirent(ec, vfs, dirptr, &ent)) {
        // If successful, mark inode as in use.
        RAMFS(vfs).inode_usage[inum] = true;
    }

    mutex_release(NULL, &RAMFS(vfs).mtx);
    return iptr;
}

// Test whether a directory is empty.
static bool is_dir_empty(fs_ramfs_inode_t *dir) {
    size_t off = 0;
    while (off < dir->len) {
        fs_ramfs_dirent_t *ent = (fs_ramfs_dirent_t *)(dir->buf + off);
        if (!cstr_equals(".", ent->name) && !cstr_equals("..", ent->name)) {
            return false;
        }
        off += ent->size;
    }
    return true;
}



// Try to mount a ramfs filesystem.
bool fs_ramfs_mount(badge_err_t *ec, vfs_t *vfs) {
    // RAMFS does not use a block device.
    if (vfs->media) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        return false;
    }

    // TODO: Parameters.
    atomic_store_explicit(&RAMFS(vfs).ram_usage, 0, memory_order_relaxed);
    RAMFS(vfs).ram_limit      = 65536;
    RAMFS(vfs).inode_list_len = 32;
    RAMFS(vfs).inode_list     = malloc(sizeof(*RAMFS(vfs).inode_list) * RAMFS(vfs).inode_list_len);
    if (!RAMFS(vfs).inode_list) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOMEM);
        return false;
    }
    RAMFS(vfs).inode_usage = malloc(sizeof(*RAMFS(vfs).inode_usage) * RAMFS(vfs).inode_list_len);
    if (!RAMFS(vfs).inode_usage) {
        free(RAMFS(vfs).inode_list);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOMEM);
        return false;
    }
    vfs->inode_root = VFS_RAMFS_INODE_ROOT;
    mutex_init(ec, &RAMFS(vfs).mtx, true, false);
    badge_err_assert_dev(ec);

    // Clear inode usage.
    mem_set(RAMFS(vfs).inode_list, 0, sizeof(*RAMFS(vfs).inode_list) * RAMFS(vfs).inode_list_len);
    mem_set(RAMFS(vfs).inode_usage, false, RAMFS(vfs).inode_list_len - 2);

    // Create root directory.
    RAMFS(vfs).inode_usage[VFS_RAMFS_INODE_ROOT] = true;
    fs_ramfs_inode_t *iptr                       = &RAMFS(vfs).inode_list[VFS_RAMFS_INODE_ROOT];

    iptr->buf   = NULL;
    iptr->len   = 0;
    iptr->cap   = 0;
    iptr->inode = VFS_RAMFS_INODE_ROOT;
    iptr->mode  = (FILETYPE_DIR << VFS_RAMFS_MODE_BIT) | 0777; /* TODO. */
    iptr->links = 1;
    iptr->uid   = 0; /* TODO. */
    iptr->gid   = 0; /* TODO. */

    fs_ramfs_dirent_t ent = {
        .size     = sizeof(ent) - VFS_RAMFS_NAME_MAX - 1 + sizeof(size_t),
        .inode    = VFS_RAMFS_INODE_ROOT,
        .name_len = 1,
        .name     = {'.', 0},
    };
    insert_dirent(ec, vfs, iptr, &ent);
    if (!badge_err_is_ok(ec)) {
        free(iptr->buf);
        free(RAMFS(vfs).inode_list);
        free(RAMFS(vfs).inode_usage);
        mutex_destroy(NULL, &RAMFS(vfs).mtx);
        return false;
    }

    ent.name_len = 2;
    ent.name[1]  = '.';
    ent.name[2]  = 0;
    insert_dirent(ec, vfs, iptr, &ent);
    if (!badge_err_is_ok(ec)) {
        free(iptr->buf);
        free(RAMFS(vfs).inode_list);
        free(RAMFS(vfs).inode_usage);
        mutex_destroy(NULL, &RAMFS(vfs).mtx);
        return false;
    }

    return true;
}

// Unmount a ramfs filesystem.
void fs_ramfs_umount(vfs_t *vfs) {
    mutex_destroy(NULL, &RAMFS(vfs).mtx);
    free(RAMFS(vfs).inode_list);
    free(RAMFS(vfs).inode_usage);
}



// Insert a new file into the given directory.
// If the file already exists, does nothing.
void fs_ramfs_create_file(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, char const *name, size_t name_len) {
    create_file(ec, vfs, dir, name, name_len, FILETYPE_REG);
}

// Insert a new directory into the given directory.
// If the file already exists, does nothing.
void fs_ramfs_create_dir(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, char const *name, size_t name_len) {
    badge_err_t ec0;
    if (!ec)
        ec = &ec0;

    // Create new directory.
    fs_ramfs_inode_t *iptr = create_file(ec, vfs, dir, name, name_len, FILETYPE_DIR);
    if (!badge_err_is_ok(ec))
        return;

    // Write . and .. entries.
    fs_ramfs_dirent_t ent = {
        .size     = sizeof(ent) - VFS_RAMFS_NAME_MAX - 1 + sizeof(size_t),
        .inode    = iptr->inode,
        .name_len = 1,
        .name     = {'.', 0},
    };
    insert_dirent(ec, vfs, iptr, &ent);
    if (!badge_err_is_ok(ec)) {
        pop_inode_refcount(vfs, iptr);
        fs_ramfs_dirent_t *ent = find_dirent(ec, vfs, RAMFILE(dir), name, name_len);
        assert_dev_drop(ent != NULL);
        remove_dirent(vfs, RAMFILE(dir), ent);
        return;
    }

    ent.name_len = 2;
    ent.name[1]  = '.';
    ent.name[2]  = 0;
    ent.inode    = dir->inode;
    insert_dirent(ec, vfs, iptr, &ent);
    if (!badge_err_is_ok(ec)) {
        pop_inode_refcount(vfs, iptr);
        fs_ramfs_dirent_t *ent = find_dirent(ec, vfs, RAMFILE(dir), name, name_len);
        assert_dev_drop(ent != NULL);
        remove_dirent(vfs, RAMFILE(dir), ent);
        return;
    }
}

// Unlink a file from the given directory.
// If this is the last reference to an inode, the inode is deleted.
void fs_ramfs_unlink(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, char const *name, size_t name_len) {
    if (name_len > VFS_RAMFS_NAME_MAX) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_TOOLONG);
    }

    // The . and .. entries can not be removed.
    if ((name_len == 1 && name[0] == '.') || (name_len == 2 && name[0] == '.' && name[1] == '.')) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_PARAM);
        return;
    }

    assert_always(mutex_acquire(ec, &RAMFS(vfs).mtx, TIMESTAMP_US_MAX));

    // Find the directory entry with the given name.
    fs_ramfs_inode_t  *dirptr = RAMFILE(dir);
    fs_ramfs_dirent_t *ent    = find_dirent(ec, vfs, dirptr, name, name_len);
    if (!ent) {
        mutex_release(NULL, &RAMFS(vfs).mtx);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOTFOUND);
        return;
    }


    // If it is also a directory, assert that it is empty.
    fs_ramfs_inode_t *iptr = &RAMFS(vfs).inode_list[ent->inode];
    if ((iptr->mode & VFS_RAMFS_MODE_MASK) == FILETYPE_DIR << VFS_RAMFS_MODE_BIT) {
        // Directories that are not empty cannot be removed.
        if (!is_dir_empty(iptr)) {
            badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOTEMPTY);
            mutex_release(NULL, &RAMFS(vfs).mtx);
            return;
        }
    }

    // Decrease inode refcount.
    pop_inode_refcount(vfs, iptr);

    // Remove directory entry.
    remove_dirent(vfs, dirptr, ent);

    mutex_release(NULL, &RAMFS(vfs).mtx);
}



// Determine the record length for converting a RAMFS dirent to a BadgerOS dirent.
// Returns the record length for a matching `dirent_t`.
static inline size_t measure_dirent(fs_ramfs_dirent_t *ent) {
    size_t ent_size  = offsetof(dirent_t, name) + ent->name_len + 1;
    ent_size        += (~ent_size + 1) % 8;
    return ent_size;
}

// Convert a RAMFS dirent to a BadgerOS dirent.
// Returns the record length for a matching `dirent_t`.
static inline size_t convert_dirent(vfs_t *vfs, dirent_t *out, fs_ramfs_dirent_t *in) {
    fs_ramfs_inode_t *iptr = &RAMFS(vfs).inode_list[in->inode];

    out->record_len  = offsetof(dirent_t, name) + in->name_len + 1;
    out->record_len += (fileoff_t)((size_t)(~out->record_len + 1) % 8);
    out->inode       = in->inode;
    out->is_dir      = (iptr->mode & VFS_RAMFS_MODE_MASK) == FILETYPE_DIR << VFS_RAMFS_MODE_BIT;
    out->is_symlink  = (iptr->mode & VFS_RAMFS_MODE_MASK) == FILETYPE_LINK << VFS_RAMFS_MODE_BIT;
    out->name_len    = (fileoff_t)in->name_len;
    mem_copy(out->name, in->name, in->name_len + 1);

    return out->record_len;
}

// Read all entries from a directory.
dirent_list_t fs_ramfs_dir_read(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir) {
    dirent_list_t res = {0};
    assert_always(mutex_acquire_shared(NULL, &RAMFS(vfs).mtx, TIMESTAMP_US_MAX));
    size_t            off  = 0;
    fs_ramfs_inode_t *iptr = RAMFILE(dir);

    // Measure required memory.
    size_t cap = 0;
    while (off < iptr->len) {
        fs_ramfs_dirent_t *ent  = (fs_ramfs_dirent_t *)(iptr->buf + off);
        cap                    += measure_dirent(ent);
        off                    += ent->size;
    }

    // Allocate memory.
    void *mem = malloc(cap);
    if (!mem) {
        mutex_release_shared(NULL, &RAMFS(vfs).mtx);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOMEM);
        return res;
    }
    res.mem  = mem;
    res.size = cap;

    // Generate entries.
    size_t out_off = 0;
    off            = 0;
    while (off < iptr->len) {
        fs_ramfs_dirent_t *in  = (fs_ramfs_dirent_t *)(iptr->buf + off);
        dirent_t          *out = (dirent_t *)((size_t)res.mem + out_off);
        convert_dirent(vfs, out, in);
        off     += in->size;
        out_off += out->record_len;
        res.ent_count++;
    }

    mutex_release_shared(NULL, &RAMFS(vfs).mtx);
    badge_err_set_ok(ec);

    return res;
}

// Atomically read the directory entry with the matching name.
// Returns true if the entry was found.
bool fs_ramfs_dir_find_ent(
    badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, dirent_t *out, char const *name, size_t name_len
) {
    fs_ramfs_inode_t  *iptr = RAMFILE(dir);
    fs_ramfs_dirent_t *in   = find_dirent(ec, vfs, iptr, name, name_len);
    if (!in) {
        return false;
    } else {
        convert_dirent(vfs, out, in);
        return true;
    }
}



// Open a file handle for the root directory.
void fs_ramfs_root_open(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *file) {
    assert_always(mutex_acquire_shared(NULL, &RAMFS(vfs).mtx, TIMESTAMP_US_MAX));

    // Install in shared file handle.
    fs_ramfs_inode_t *iptr = &RAMFS(vfs).inode_list[VFS_RAMFS_INODE_ROOT];
    RAMFILE(file)          = iptr;
    file->inode            = VFS_RAMFS_INODE_ROOT;
    file->refcount         = 1;
    file->type             = FILETYPE_DIR;

    iptr->links++;

    mutex_release_shared(NULL, &RAMFS(vfs).mtx);
    badge_err_set_ok(ec);
}

// Open a file for reading and/or writing.
void fs_ramfs_file_open(
    badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *dir, vfs_file_obj_t *file, char const *name, size_t name_len
) {
    assert_always(mutex_acquire(NULL, &RAMFS(vfs).mtx, TIMESTAMP_US_MAX));

    // Look up the file in question.
    fs_ramfs_inode_t  *dirptr = RAMFILE(dir);
    fs_ramfs_dirent_t *ent    = find_dirent(ec, vfs, dirptr, name, name_len);
    if (!badge_err_is_ok(ec)) {
        mutex_release(NULL, &RAMFS(vfs).mtx);
        return;
    }
    if (!ent) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_NOTFOUND);
        mutex_release(NULL, &RAMFS(vfs).mtx);
        return;
    }

    // Increase refcount.
    fs_ramfs_inode_t *iptr = &RAMFS(vfs).inode_list[ent->inode];
    iptr->links++;

    // Install in shared file handle.
    RAMFILE(file)  = iptr;
    file->inode    = iptr->inode;
    file->vfs      = vfs;
    file->refcount = 1;
    file->size     = (fileoff_t)iptr->len;
    file->type     = (iptr->mode & VFS_RAMFS_MODE_MASK) >> VFS_RAMFS_MODE_BIT;

    mutex_release(NULL, &RAMFS(vfs).mtx);
    badge_err_set_ok(ec);
}

// Close a file opened by `fs_ramfs_file_open`.
// Only raises an error if `file` is an invalid file descriptor.
void fs_ramfs_file_close(vfs_t *vfs, vfs_file_obj_t *file) {
    assert_always(mutex_acquire(NULL, &RAMFS(vfs).mtx, TIMESTAMP_US_MAX));
    pop_inode_refcount(vfs, RAMFILE(file));
    mutex_release(NULL, &RAMFS(vfs).mtx);
    RAMFILE(file) = NULL;
}

// Read bytes from a file.
void fs_ramfs_file_read(
    badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *file, fileoff_t offset, uint8_t *readbuf, fileoff_t readlen
) {
    if (offset < 0) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_RANGE);
        return;
    }
    assert_always(mutex_acquire_shared(NULL, &RAMFS(vfs).mtx, TIMESTAMP_US_MAX));

    fs_ramfs_inode_t *iptr = RAMFILE(file);

    // Bounds check file and read offsets.
    if (offset + readlen > (ptrdiff_t)iptr->len || offset + readlen < offset) {
        mutex_release_shared(NULL, &RAMFS(vfs).mtx);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_RANGE);
        return;
    }

    // Checks passed, return data.
    mem_copy(readbuf, iptr->buf + offset, readlen);
    mutex_release_shared(NULL, &RAMFS(vfs).mtx);
    badge_err_set_ok(ec);
}

// Write bytes from a file.
void fs_ramfs_file_write(
    badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *file, fileoff_t offset, uint8_t const *writebuf, fileoff_t writelen
) {
    if (offset < 0) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_RANGE);
        return;
    }
    assert_always(mutex_acquire(NULL, &RAMFS(vfs).mtx, TIMESTAMP_US_MAX));

    fs_ramfs_inode_t *iptr = RAMFILE(file);

    // Bounds check file and read offsets.
    if (offset + writelen > (ptrdiff_t)iptr->len || offset + writelen < offset) {
        mutex_release(ec, &RAMFS(vfs).mtx);
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_RANGE);
        return;
    }

    // Checks passed, update data.
    mem_copy(iptr->buf + offset, writebuf, writelen);
    mutex_release(ec, &RAMFS(vfs).mtx);
}

// Change the length of a file opened by `fs_ramfs_file_open`.
void fs_ramfs_file_resize(badge_err_t *ec, vfs_t *vfs, vfs_file_obj_t *file, fileoff_t new_size) {
    if (new_size < 0) {
        badge_err_set(ec, ELOC_FILESYSTEM, ECAUSE_RANGE);
        return;
    }
    assert_always(mutex_acquire(NULL, &RAMFS(vfs).mtx, TIMESTAMP_US_MAX));

    // Attempt to resize the buffer.
    fs_ramfs_inode_t *iptr     = RAMFILE(file);
    fileoff_t         old_size = (fileoff_t)iptr->len;
    if (resize_inode(ec, vfs, iptr, new_size)) {
        file->size = new_size;
        if (new_size > old_size) {
            // Zero out new bits.
            mem_set(iptr->buf + old_size, 0, new_size - old_size);
        }
    }

    mutex_release(NULL, &RAMFS(vfs).mtx);
}



// Commit all pending writes to disk.
// The filesystem, if it does caching, must always sync everything to disk at once.
void fs_ramfs_flush(badge_err_t *ec, vfs_t *vfs) {
    // RAMFS does not do caching, so flush does nothing.
    (void)vfs;
    badge_err_set_ok(ec);
}



// RAMFS vtable.
static vfs_vtable_t ramfs_vtable = {
    .mount        = fs_ramfs_mount,
    .umount       = fs_ramfs_umount,
    .create_file  = fs_ramfs_create_file,
    .create_dir   = fs_ramfs_create_dir,
    .unlink       = fs_ramfs_unlink,
    .dir_read     = fs_ramfs_dir_read,
    .dir_find_ent = fs_ramfs_dir_find_ent,
    .root_open    = fs_ramfs_root_open,
    .file_open    = fs_ramfs_file_open,
    .file_close   = fs_ramfs_file_close,
    .file_read    = fs_ramfs_file_read,
    .file_write   = fs_ramfs_file_write,
    .file_resize  = fs_ramfs_file_resize,
    .flush        = fs_ramfs_flush,
};

// RAMFS declaration.
FS_DRIVER_DECL(ramfs_driver) = {
    .vtable           = &ramfs_vtable,
    .id               = "ramfs",
    .file_cookie_size = sizeof(fs_ramfs_file_t),
    .vfs_cookie_size  = sizeof(fs_ramfs_t),
};
