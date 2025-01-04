
// SPDX-License-Identifier: MIT

#include "filesystem/syscall_impl.h"

#include "filesystem.h"
#include "process/internal.h"



// Open a file, optionally relative to a directory.
// Returns <= -1 on error, file descriptor number of success.
file_t syscall_fs_open(file_t dirfd, char const *path, size_t path_len, int oflags) {
    process_t *const proc = proc_current();
    file_t           fd   = fs_open(NULL, dirfd, path, path_len, oflags);
    file_t           virt = -1;
    if (fd >= 0) {
        badge_err_t ec;
        virt = proc_add_fd_raw(&ec, proc, fd);
        if (!badge_err_is_ok(&ec)) {
            fs_close(NULL, fd);
            virt = -1;
        }
    }
    return virt;
}

// Flush and close a file.
bool syscall_fs_close(file_t virt) {
    process_t *const proc = proc_current();
    badge_err_t      ec;
    file_t           fd = proc_find_fd_raw(NULL, proc_current(), virt);
    proc_remove_fd_raw(&ec, proc, virt);
    if (!badge_err_is_ok(&ec)) {
        return false;
    } else {
        fs_close(NULL, fd);
        return true;
    }
}

// Read bytes from a file.
// Returns -1 on EOF, <= -2 on error, read count on success.
long syscall_fs_read(file_t virt, void *read_buf, long read_len) {
    file_t fd = proc_find_fd_raw(NULL, proc_current(), virt);
    if (fd != -1) {
        return fs_read(NULL, fd, read_buf, read_len);
    }
    return -1;
}

// Write bytes to a file.
// Returns <= -1 on error, write count on success.
long syscall_fs_write(file_t virt, void const *write_buf, long write_len) {
    file_t fd = proc_find_fd_raw(NULL, proc_current(), virt);
    if (fd != -1) {
        return fs_write(NULL, fd, write_buf, write_len);
    }
    return -1;
}

// Read directory entries from a directory handle.
// See `dirent_t` for the format.
// Returns <= -1 on error, read count on success.
long syscall_fs_getdents(file_t virt, void *read_buf, long read_len) {
    file_t fd = proc_find_fd_raw(NULL, proc_current(), virt);
    fs_seek(NULL, fd, 0, SEEK_ABS);
    return fs_read(NULL, fd, read_buf, read_len);
}
