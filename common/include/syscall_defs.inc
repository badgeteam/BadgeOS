
// SPDX-License-Identifier: MIT
// clang-format off



/* ==== DEFAULT MACRO EXPANSIONS ==== */

// Normal syscall definition.
#ifndef SYSCALL_DEF
#define SYSCALL_DEF(no, enum, name, returns, ...)
#endif

// Normal syscall definition (returning void).
#ifndef SYSCALL_DEF_V
#define SYSCALL_DEF_V(no, enum, name, ...) SYSCALL_DEF(no, enum, name, void, __VA_ARGS__)
#endif

// Fast syscall definition.
#ifndef SYSCALL_DEF_F
#define SYSCALL_DEF_F(no, enum, name, returns, ...) SYSCALL_DEF(no, enum, name, returns, __VA_ARGS__)
#endif

// Fast syscall definition (returning void).
#ifndef SYSCALL_DEF_FV
#define SYSCALL_DEF_FV(no, enum, name, ...) SYSCALL_DEF_F(no, enum, name, void, __VA_ARGS__)
#endif



/* ==== THREAD SYSCALLS ==== */
// Implemented in process/syscall_impl.c

// Yield to other threads.
SYSCALL_DEF_V(1, SYSCALL_THREAD_YIELD, syscall_thread_yield)

// // Create a new thread.
// SYSCALL_DEF(2, SYSCALL_THREAD_CREATE, syscall_thread_create, bool, void *entry, void *arg, int priority)

// // Suspend a thread; it will not run again until resumed.
// SYSCALL_DEF(3, SYSCALL_THREAD_SUSPEND, syscall_thread_suspend, bool, tid_t thread)

// // Resume a thread; this does nothing if it is already running.
// SYSCALL_DEF(4, SYSCALL_THREAD_RESUME, syscall_thread_resume, bool, tid_t thread)

// // Detach a thread; the thread will be destroyed as soon as it exits.
// SYSCALL_DEF(5, SYSCALL_THREAD_DETACH, syscall_thread_detach, bool, tid_t thread)

// // Destroy a thread; it will be stopped and its resources released.
// SYSCALL_DEF(6, SYSCALL_THREAD_DESTROY, syscall_thread_destroy, bool, tid_t thread)

// // Exit the current thread; exit code can be read unless destroyed or detached.
// SYSCALL_DEF_V(7, SYSCALL_THREAD_EXIT, syscall_thread_exit, int code)



/* ==== PROCESS MANAGEMENT SYSCALLS ==== */
// Implemented in process/syscall_impl.c

// Exit the process; exit code can be read by parent process.
SYSCALL_DEF_V(8, SYSCALL_PROC_EXIT, syscall_proc_exit, int code)

// Get the command-line arguments (i.e. argc+argv) of the current process.
// If memory is large enough, a NULL-terminated argv array of C-string pointers and their data is stored in `memory`.
// The function returns how many bytes would be needed to store the structure.
// If the memory was not large enough, it it not modified.
SYSCALL_DEF(9, SYSCALL_PROC_GETARGS, syscall_proc_getargs, size_t, size_t cap, void *memory)

// Create a "pre-start" child process that inherits this process' file descriptors and environment variables.
// The child process is created in a halted state and it is killed if the parent exits before the child is resumed.
// Returns process ID of new child on success, or a (negative) errno on failure.
SYSCALL_DEF(10, SYSCALL_PROC_PCREATE, syscall_proc_pcreate, int, char const *binary, int argc, char const *const *argv)

// Destroy a "pre-start" child process.
// Usually used in case of errors.
SYSCALL_DEF(11, SYSCALL_PROC_PDESTROY, syscall_proc_pdestroy, bool, int child)

// Starts a "pre-start" child process, thereby converting it into a running child process.
SYSCALL_DEF(12, SYSCALL_PROC_PSTART, syscall_proc_pstart, bool, int child)

// Set the signal handler for a specific signal number.
SYSCALL_DEF(13, SYSCALL_PROC_SIGHANDLER, syscall_proc_sighandler, void *, int signum, void *newhandler)

// Return from a signal handler.
SYSCALL_DEF_V(14, SYSCALL_PROC_SIGRET, syscall_proc_sigret)

// Get child process status update.
SYSCALL_DEF(15, SYSCALL_PROC_WAITPID, syscall_proc_waitpid, int, int pid, int *wstatus, int options)


/* ==== FILESYSTEM SYSCALLS ==== */
// Implemented in filesystem/syscall_impl.c

// Open a file, optionally relative to a directory.
// Returns <= -1 on error, file descriptor number on success.
SYSCALL_DEF(16, SYSCALL_FS_OPEN, syscall_fs_open, file_t, char const *path, file_t relative_to, int oflags)

// Flush and close a file.
SYSCALL_DEF(17, SYSCALL_FS_CLOSE, syscall_fs_close, bool, file_t fd)

// Read bytes from a file.
// Returns -1 on EOF, <= -2 on error, read count on success.
SYSCALL_DEF(18, SYSCALL_FS_READ, syscall_fs_read, long, file_t fd, void *read_buf, long read_len)

// Write bytes to a file.
// Returns <= -1 on error, write count on success.
SYSCALL_DEF(19, SYSCALL_FS_WRITE, syscall_fs_write, long, file_t fd, void const *write_buf, long write_len)

// Read directory entries from a directory handle.
// See `dirent_t` for the format.
// Returns <= -1 on error, read count on success.
SYSCALL_DEF(20, SYSCALL_FS_GETDENTS, syscall_fs_getdents, long, file_t fd, void *read_buf, long read_len)

// // Rename and/or move a file to another path, optionally relative to one or two directories.
// SYSCALL_DEF_V(21, SYSCALL_FS_RENAME, syscall_fs_rename)

// // Get file status given file handler or path, optionally following the final symlink.
// SYSCALL_DEF_V(22, SYSCALL_FS_STAT, syscall_fs_stat)



/* ==== MEMORY MANAGEMENT SYSCALLS ==== */
// Implemented in process/syscall_impl.c

// Map a new range of memory at an arbitrary virtual address.
// This may round up to a multiple of the page size.
// Alignment may be less than `align` if the kernel doesn't support it.
SYSCALL_DEF(23, SYSCALL_MEM_ALLOC, syscall_mem_alloc, void *, size_t vaddr, size_t size, size_t align, int flags)

// Get the size of a range of memory previously allocated with `SYSCALL_MEM_ALLOC`.
// Returns 0 if there is no range starting at the given address.
SYSCALL_DEF(24, SYSCALL_MEM_SIZE, syscall_mem_size, size_t, void *address)

// Unmap a range of memory previously allocated with `SYSCALL_MEM_ALLOC`.
// Returns whether a range of memory was unmapped.
SYSCALL_DEF(25, SYSCALL_MEM_DEALLOC, syscall_mem_dealloc, bool, void *address)



/* ==== LOW-LEVEL HAL SYSCALLS ==== */
// Implemented in hal/syscall_impl.c

// Returns the amount of GPIO pins present.
// Cannot produce an error.
SYSCALL_DEF(26, SYSCALL_IO_COUNT, syscall_io_count, int)
// Sets the mode of GPIO pin `pin` to `mode`.
SYSCALL_DEF_V(27, SYSCALL_IO_MODE, syscall_io_mode, badge_err_t *ec, int pin, io_mode_t mode)
// Get the mode of GPIO pin `pin`.
SYSCALL_DEF(28, SYSCALL_IO_GETMODE, syscall_io_getmode, io_mode_t, badge_err_t *ec, int pin)
// Sets the pull resistor behaviour of GPIO pin `pin` to `dir`.
SYSCALL_DEF_V(29, SYSCALL_IO_PULL, syscall_io_pull, badge_err_t *ec, int pin, io_pull_t dir)
// Get the  pull resistor behaviour of GPIO pin `pin`.
SYSCALL_DEF(30, SYSCALL_IO_GETPULL, syscall_io_getpull, io_pull_t, badge_err_t *ec, int pin)
// Writes level to GPIO pin pin.
SYSCALL_DEF_V(31, SYSCALL_IO_WRITE, syscall_io_write, badge_err_t *ec, int pin, bool level)
// Reads logic level value from GPIO pin `pin`.
// Returns false on error.
SYSCALL_DEF(32, SYSCALL_IO_READ, syscall_io_read, bool, badge_err_t *ec, int pin)
// Determine whether GPIO `pin` is claimed by a peripheral.
// Returns false on error.
SYSCALL_DEF(33, SYSCALL_IO_IS_PERIPHERAL, syscall_io_is_peripheral, bool, badge_err_t *ec, int pin)

// Returns the amount of I²C peripherals present.
// Cannot produce an error.
SYSCALL_DEF(34, SYSCALL_I2C_COUNT, syscall_i2c_count, int)
// Initialises I²C peripheral i2c_num in slave mode with SDA pin `sda_pin`, SCL pin `scl_pin` and clock speed/bitrate
// bitrate. When initialised as an I²C master, the modes of the SDA and SCL pins are changed automatically. This
// function may be used again to change the settings on an initialised I²C peripheral in master mode.
SYSCALL_DEF_V(35, SYSCALL_I2C_MASTER_INIT, syscall_i2c_master_init, badge_err_t *ec, int i2c_num, int sda_pin, int scl_pin, int32_t bitrate)
// De-initialises I²C peripheral i2c_num in master mode.
SYSCALL_DEF_V(36, SYSCALL_I2C_MASTER_DEINIT, syscall_i2c_master_deinit, badge_err_t *ec, int i2c_num)
// Reads len bytes into buffer buf from I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged read bytes.
SYSCALL_DEF(37, SYSCALL_I2C_MASTER_READ_FROM, syscall_i2c_master_read_from, size_t, badge_err_t *ec, int i2c_num, int slave_id, void *buf, size_t len)
// Writes len bytes from buffer buf to I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged written bytes.
SYSCALL_DEF(38, SYSCALL_I2C_MASTER_WRITE_TO, syscall_i2c_master_write_to, size_t, badge_err_t *ec, int i2c_num, int slave_id, void const *buf, size_t len)

// Returns the amount of SPI peripherals present.
// Cannot produce an error.
SYSCALL_DEF(39, SYSCALL_SPI_COUNT, syscall_spi_count, int)
// Initialises SPI peripheral spi_num in controller mode with SCLK pin `sclk_pin`, MOSI pin `mosi_pin`, MISO pin
// `miso_pin`, SS pin `ss_pin` and clock speed/bitrate bitrate. The modes of the SCLK, MOSI, MISO and SS pins are
// changed automatically. This function may be used again to change the settings on an initialised SPI peripheral in
// controller mode.
SYSCALL_DEF_V(40, SYSCALL_SPI_CONTROLLER_INIT, syscall_spi_controller_init, badge_err_t *ec, int spi_num, int sclk_pin, int mosi_pin, int miso_pin, int ss_pin, int32_t bitrate)
// De-initialises SPI peripheral.
SYSCALL_DEF_V(41, SYSCALL_SPI_DEINIT, syscall_spi_deinit, badge_err_t *ec, int spi_num)
// Reads len bytes into buffer buf from SPI device.
// This function blocks until the entire transaction is completed.
SYSCALL_DEF_V(42, SYSCALL_SPI_CONTROLLER_READ, syscall_spi_controller_read, badge_err_t *ec, int spi_num, void *buf, size_t len)
// Writes len bytes from buffer buf to SPI device.
// This function blocks until the entire transaction is completed.
SYSCALL_DEF_V(43, SYSCALL_SPI_CONTROLLER_WRITE, syscall_spi_controller_write, badge_err_t *ec, int spi_num, void const *buf, size_t len)
// Writes len bytes from buffer buf to SPI device, then reads len bytes into buffer buf from SPI device.
// Write and read happen simultaneously if the full-duplex flag fdx is set.
// This function blocks until the entire transaction is completed.
SYSCALL_DEF_V(44, SYSCALL_SPI_CONTROLLER_TRANSFER, syscall_spi_controller_transfer, badge_err_t *ec, int spi_num, void *buf, size_t len, bool fdx)




/* ==== SYSTEM MANAGEMENT SYSCALLS ==== */
// Implemented in main.c

// Start the shutdown process.
SYSCALL_DEF_V(45, SYSCALL_SYS_SHUTDOWN, syscall_sys_shutdown, bool is_reboot)



/* ==== TEMPORARY SYSCALLS ==== */
// Implemented in process/syscall_impl.c

// TEMPORARY: Write one or more characters to the log.
SYSCALL_DEF_V(46, SYSCALL_TEMP_WRITE, syscall_temp_write, char const *message, size_t length)



#undef SYSCALL_DEF
#undef SYSCALL_DEF_V
#undef SYSCALL_DEF_F
#undef SYSCALL_DEF_FV
