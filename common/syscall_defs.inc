
// SPDX-License-Identifier: MIT

/* ==== TEMPORARY SYSCALLS ==== */
// Implemented in syscall.c

// TEMPORARY: Write one or more characters to the log.
SYSCALL_DEF(0xff00, SYSCALL_TEMP_WRITE, syscall_temp_write, void, char const *message, size_t length)



/* ==== THREAD SYSCALLS ==== */
// Implemented in process/syscall_impl.c

// Yield to other threads.
SYSCALL_DEF(0x0100, SYSCALL_THREAD_YIELD, syscall_thread_yield, void)

// Create a new thread.
SYSCALL_DEF(0x0101, SYSCALL_THREAD_CREATE, syscall_thread_create, bool, void *entry, void *arg, int priority)

// Suspend a thread; it will not run again until resumed.
SYSCALL_DEF(0x0102, SYSCALL_THREAD_SUSPEND, syscall_thread_suspend, bool, tid_t thread)

// Resume a thread; this does nothing if it is already running.
SYSCALL_DEF(0x0103, SYSCALL_THREAD_RESUME, syscall_thread_resume, bool, tid_t thread)

// Detach a thread; the thread will be destroyed as soon as it exits.
SYSCALL_DEF(0x0104, SYSCALL_THREAD_DETACH, syscall_thread_detach, bool, tid_t thread)

// Destroy a thread; it will be stopped and its resources released.
SYSCALL_DEF(0x0105, SYSCALL_THREAD_DESTROY, syscall_thread_destroy, bool, tid_t thread)

// Exit the current thread; exit code can be read unless destroyed.
SYSCALL_DEF(0x0106, SYSCALL_THREAD_EXIT, syscall_thread_exit, void, int code)



/* ==== SELF MANAGEMENT SYSCALLS ==== */
// Implemented in process/syscall_impl.c

// Exit the process; exit code can be read by parent process.
SYSCALL_DEF(0x0200, SYSCALL_SELF_EXIT, syscall_self_exit, void, int code)



/* ==== FILESYSTEM SYSCALLS ==== */
// Implemented in filesystem/syscall_impl.c

// Open a file, optionally relative to a directory.
// Returns <= -1 on error, file descriptor number of success.
SYSCALL_DEF(0x0300, SYSCALL_FS_OPEN, syscall_fs_open, file_t, char const *path, file_t relative_to, int oflags)

// Flush and close a file.
SYSCALL_DEF(0x0301, SYSCALL_FS_CLOSE, syscall_fs_close, bool, file_t fd)

// Read bytes from a file.
// Returns -1 on EOF, <= -2 on error, read count on success.
SYSCALL_DEF(0x0302, SYSCALL_FS_READ, syscall_fs_read, long, file_t fd, void *read_buf, long read_len)

// Write bytes to a file.
// Returns <= -1 on error, write count on success.
SYSCALL_DEF(0x0303, SYSCALL_FS_WRITE, syscall_fs_write, long, file_t fd, void const *write_buf, long write_len)

// Read directory entries from a directory handle.
// See `dirent_t` for the format.
// Returns <= -1 on error, read count on success.
SYSCALL_DEF(0x0304, SYSCALL_FS_GETDENTS, syscall_fs_getdents, long, file_t fd, void *read_buf, long read_len)

// RESERVED FOR: Rename and/or move a file to another path, optionally relative to one or two directories.
SYSCALL_DEF(0x0305, SYSCALL_FS_RENAME, syscall_fs_rename, void)

// RESERVED FOR: Get file status given file handler or path, optionally following the final symlink.
SYSCALL_DEF(0x0306, SYSCALL_FS_STAT, syscall_fs_stat, void)



/* ==== MEMORY MANAGEMENT SYSCALLS ==== */
// Implemented in process/syscall_impl.c

// Map a new range of memory at an arbitrary virtual address.
// This may round up to a multiple of the page size.
// Alignment may be less than `align` if the kernel doesn't support it.
SYSCALL_DEF(0x0400, SYSCALL_MEM_ALLOC, syscall_mem_alloc, void *, size_t vaddr, size_t size, size_t align, int flags)

// Get the size of a range of memory previously allocated with `SYSCALL_MEM_ALLOC`.
// Returns 0 if there is no range starting at the given address.
SYSCALL_DEF(0x0401, SYSCALL_MEM_SIZE, syscall_mem_size, size_t, void *address)

// Unmap a range of memory previously allocated with `SYSCALL_MEM_ALLOC`.
// Returns whether a range of memory was unmapped.
SYSCALL_DEF(0x0402, SYSCALL_MEM_DEALLOC, syscall_mem_dealloc, bool, void *address)



/* ==== PROCESS MANAGEMENT SYSCALLS ==== */



/* ==== SYSTEM MANAGEMENT SYSCALLS ==== */
// Implemented in main.c

// Start the shutdown process.
SYSCALL_DEF(0x0500, SYSCALL_SYS_SHUTDOWN, syscall_sys_shutdown, void, bool is_reboot)



#undef SYSCALL_DEF
