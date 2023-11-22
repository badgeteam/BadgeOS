
// SPDX-License-Identifier: MIT

// System call definitions.
#ifdef __ASSEMBLER__
#define SYSCALL_DEF(num, name) .equ SYSCALL_##name, num
#else
#define SYSCALL_DEF(num, name) SYSCALL_##name = num,
typedef enum {
#endif



/* ==== TEMPORARY SYSCALLS ==== */
// TEMPORARY: Write one or more characters to the log.
// void(char const *message, size_t length)
SYSCALL_DEF(0xff00, TEMP_WRITE)



/* ==== THREAD SYSCALLS ==== */
// Yield to other threads.
// void(void)
SYSCALL_DEF(0x0100, THREAD_YIELD)

// Create a new thread.
// bool(void *entry, void *arg, int priority)
SYSCALL_DEF(0x0101, THREAD_CREATE)

// Suspend a thread; it will not run again until resumed.
// bool(tid_t thread)
SYSCALL_DEF(0x0102, THREAD_SUSPEND)

// Resume a thread; this does nothing if it is already running.
// bool(tid_t thread)
SYSCALL_DEF(0x0103, THREAD_RESUME)

// Detach a thread; the thread will be destroyed as soon as it exits.
// bool(tid_t thread)
SYSCALL_DEF(0x0104, THREAD_DETACH)

// Destroy a thread; it will be stopped and its resources released.
// bool(tid_t thread)
SYSCALL_DEF(0x0105, THREAD_DESTROY)

// Exit the current thread; exit code can be read unless destroyed.
// void(int code) NORETURN
SYSCALL_DEF(0x0106, THREAD_EXIT)



/* ==== SELF MANAGEMENT SYSCALLS ==== */
// Exit the process; exit code can be read by parent process.
// void(int code) NORETURN
SYSCALL_DEF(0x0200, SELF_EXIT)



/* ==== FILESYSTEM SYSCALLS ==== */
// Open a file, optionally relative to a directory.
// Returns <= -1 on error, file descriptor number of success.
// file_t(char const *path, file_t relative_to, int oflags)
SYSCALL_DEF(0x0300, FS_OPEN)

// Flush and close a file.
// bool(file_t fd)
SYSCALL_DEF(0x0301, FS_CLOSE)

// Read bytes from a file.
// Returns -1 on EOF, <= -2 on error, read count on success.
// long(file_t fd, void *read_buf, long read_len)
SYSCALL_DEF(0x0302, FS_READ)

// Write bytes to a file.
// Returns <= -1 on error, write count on success.
// long(file_t fd, void const *write_buf, long write_len)
SYSCALL_DEF(0x0303, FS_WRITE)

// Read directory entries from a directory handle.
// See `dirent_t` for the format.
// Returns <= -1 on error, read count on success.
// long(file_t fd, void *read_buf, long read_len)
SYSCALL_DEF(0x0304, FS_GETDENTS)

// RESERVED FOR: Rename and/or move a file to another path, optionally relative to one or two directories.
SYSCALL_DEF(0x0305, FS_RENAME)

// RESERVED FOR: Get file status given file handler or path, optionally following the final symlink.
SYSCALL_DEF(0x0306, FS_STAT)



/* ==== PROCESS MANAGEMENT SYSCALLS ==== */



#ifndef __ASSEMBLER__
}
syscall_num_t;
#endif
