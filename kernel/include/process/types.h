
// SPDX-License-Identifier: MIT

#pragma once

#include "filesystem.h"
#include "kbelf.h"
#include "list.h"
#include "mutex.h"
#include "port/hardware_allocation.h"
#include "port/memprotect.h"
#include "scheduler/scheduler.h"
#include "signal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



// A memory map entry.
typedef struct {
    // Base physical address of the region.
    size_t paddr;
#if MEMMAP_VMEM
    // Base virtual address of the region.
    size_t vaddr;
#endif
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
    mpu_ctx_t mpu_ctx;
    // Number of mapped regions.
    size_t    regions_len;
#ifdef PROC_MEMMAP_MAX_REGIONS
    // Mapped regions.
    proc_memmap_ent_t regions[PROC_MEMMAP_MAX_REGIONS];
#else
    // Capacity for mapped regions.
    size_t             regions_cap;
    // Mapped regions.
    proc_memmap_ent_t *regions;
#endif
} proc_memmap_t;

// Process file descriptor.
typedef struct {
    file_t virt;
    file_t real;
} proc_fd_t;

// Pending signal entry.
typedef struct {
    // Doubly-linked list node.
    dlist_node_t node;
    // Signal number.
    int          signum;
} sigpending_t;

// Globally unique process ID.
typedef int pid_t;

// A process and all of its resources.
typedef struct process_t {
    // Node for child process list.
    dlist_node_t  node;
    // Parent process, NULL for process 1.
    process_t    *parent;
    // Process binary.
    char const   *binary;
    // Number of arguments.
    int           argc;
    // Value of arguments.
    char        **argv;
    // Size required to store all of argv.
    size_t        argv_size;
    // Number of file descriptors.
    size_t        fds_len;
    // File descriptors.
    proc_fd_t    *fds;
    // Number of threads.
    size_t        threads_len;
    // Thread handles.
    tid_t        *threads;
    // Process ID.
    pid_t         pid;
    // Memory map information.
    proc_memmap_t memmap;
    // Resource mutex used for multithreading processes.
    mutex_t       mtx;
    // Process status flags.
    atomic_int    flags;
    // Pending signals list.
    dlist_t       sigpending;
    // Child process list.
    dlist_t       children;
    // Signal handler virtual addresses.
    // First index is for signal handler returns.
    size_t        sighandlers[SIG_COUNT];
    // Exit code if applicable.
    int           state_code;
    // Total time usage.
    timeusage_t   timeusage;
} process_t;
