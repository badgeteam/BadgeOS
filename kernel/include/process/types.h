
// SPDX-License-Identifier: MIT

#pragma once

#include "filesystem.h"
#include "kbelf.h"
#include "memprotect.h"
#include "mutex.h"
#include "port/hardware_allocation.h"
#include "scheduler/scheduler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



#define PROC_MTX_TIMEOUT 50000

// A memory map entry.
typedef struct {
    // Base address of the region.
    size_t base;
    // Size of the region.
    size_t size;
    // Write permission.
    bool   write;
    // Execution permission.
    bool   exec;
} proc_memmap_ent_t;

// Process memory map information.
typedef struct proc_memmap_t {
    // Memory management cache.
    mpu_ctx_t         mpu_ctx;
    // Number of mapped regions.
    size_t            regions_len;
    // Mapped regions.
    proc_memmap_ent_t regions[PROC_MEMMAP_MAX_REGIONS];
} proc_memmap_t;

// Process file descriptor.
typedef struct {
    int    virt;
    file_t real;
} proc_fd_t;

// Globally unique process ID.
typedef int pid_t;

// A process and all of its resources.
typedef struct process_t {
    // Number of arguments.
    int              argc;
    // Value of arguments.
    char           **argv;
    // Number of file descriptors.
    size_t           fds_len;
    // File descriptors.
    proc_fd_t       *fds;
    // Number of threads.
    size_t           threads_len;
    // Thread handles.
    sched_thread_t **threads;
    // Process ID.
    pid_t            pid;
    // Memory map information.
    proc_memmap_t    memmap;
    // Resource mutex used for multithreading processes.
    mutex_t          mtx;
    // Process status flags.
    atomic_int       flags;
    // Exit code if applicable.
    int              exit_code;
} process_t;
