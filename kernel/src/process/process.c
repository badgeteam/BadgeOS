
// SPDX-License-Identifier: MIT

#include "process/process.h"

#include "arrays.h"
#include "badge_strings.h"
#include "cpu/panic.h"
#include "housekeeping.h"
#include "isr_ctx.h"
#include "kbelf.h"
#include "log.h"
#include "malloc.h"
#include "memprotect.h"
#include "port/hardware_allocation.h"
#include "port/port.h"
#include "process/internal.h"
#include "process/sighandler.h"
#include "process/types.h"
#include "scheduler/cpu.h"
#include "scheduler/types.h"
#include "static-buddy.h"
#include "usercopy.h"

#include <stdatomic.h>


// Globally unique PID number counter.
static pid_t       pid_counter = 1;
// Global process lifetime mutex.
static mutex_t     proc_mtx    = MUTEX_T_INIT_SHARED;
// Number of processes.
static size_t      procs_len   = 0;
// Capacity for processes.
static size_t      procs_cap   = 0;
// Process array.
static process_t **procs       = NULL;
extern atomic_int  kernel_shutdown_mode;
// Allow process 1 to die without kernel panic.
static bool        allow_proc1_death() {
    return kernel_shutdown_mode && procs_len == 1;
}

// Set arguments for a process.
// If omitted, argc will be 0 and argv will be NULL.
static bool proc_setargs_raw_unsafe(badge_err_t *ec, process_t *process, int argc, char const *const *argv);



// Clean up: the housekeeping task.
static void clean_up_from_housekeeping(int taskno, void *arg) {
    (void)taskno;
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    process_t *proc = proc_get_unsafe((int)arg);

    // Delete run-time resources.
    proc_delete_runtime_raw(proc);
    if (!proc->parent) {
        // Init process during shutdown; delete right away.
        mutex_release_shared(NULL, &proc_mtx);
        proc_delete((int)arg);
        return;
    }

    // Check whether parent ignores SIGCHLD.
    mutex_acquire_shared(NULL, &proc->parent->mtx, TIMESTAMP_US_MAX);
    bool ignored = proc->parent->sighandlers[SIGCHLD] == SIG_IGN;
    mutex_release_shared(NULL, &proc->parent->mtx);

    if (ignored) {
        // Parent process ignores SIGCHLD; delete right away.
        mutex_release_shared(NULL, &proc_mtx);
        proc_delete((int)arg);
    } else {
        // Signal parent process.
        atomic_fetch_or(&proc->flags, PROC_STATECHG);
        proc_raise_signal_raw(NULL, proc->parent, SIGCHLD);
        mutex_release_shared(NULL, &proc_mtx);
    }
}

// Kill a process from one of its own threads.
void proc_exit_self(int code) {
    // Mark this process as exiting.
    sched_thread_t *thread  = sched_get_current_thread();
    process_t      *process = thread->process;
    mutex_acquire(NULL, &process->mtx, TIMESTAMP_US_MAX);
    atomic_fetch_or(&process->flags, PROC_EXITING);
    process->state_code = code;
    mutex_release(NULL, &process->mtx);

    // Add deleting runtime to the housekeeping list.
    assert_always(hk_add_once(0, clean_up_from_housekeeping, (void *)process->pid) != -1);
}


// Compare processes by ID.
int proc_sort_pid_cmp(void const *a, void const *b) {
    return (*(process_t **)a)->pid - (*(process_t **)b)->pid;
}

// Create a new, empty process.
process_t *proc_create_raw(badge_err_t *ec, pid_t parentpid, char const *binary, int argc, char const *const *argv) {
    // Get a new PID.
    mutex_acquire(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    process_t *parent = proc_get_unsafe(parentpid);
    if (pid_counter == 1) {
        assert_dev_drop(parentpid <= 0);
    } else if (!parent) {
        mutex_release(NULL, &proc_mtx);
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
        return NULL;
    }

    // Allocate a process entry.
    process_t *handle = malloc(sizeof(process_t));
    if (!handle) {
        mutex_release(NULL, &proc_mtx);
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return NULL;
    }

    // Install default values.
    *handle = (process_t){
        .node        = DLIST_NODE_EMPTY,
        .parent      = parent,
        .binary      = binary,
        .argc        = 0,
        .argv        = NULL,
        .argv_size   = 0,
        .fds_len     = 0,
        .fds         = NULL,
        .threads_len = 0,
        .threads     = NULL,
        .pid         = pid_counter,
        .memmap =
            {
                .regions_len = 0,
            },
        .mtx        = MUTEX_T_INIT_SHARED,
        .flags      = PROC_PRESTART,
        .sigpending = DLIST_EMPTY,
        .children   = DLIST_EMPTY,
    };

    // Set default signal handlers.
    for (size_t i = 0; i < SIG_COUNT; i++) {
        handle->sighandlers[i] = SIG_DFL;
    }

    // Install arguments.
    if (!proc_setargs_raw_unsafe(ec, handle, argc, argv)) {
        free(handle);
        mutex_release(NULL, &proc_mtx);
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return NULL;
    }

    // Insert the entry into the list.
    array_binsearch_t res = array_binsearch(procs, sizeof(process_t *), procs_len, &handle, proc_sort_pid_cmp);
    if (!array_lencap_insert(&procs, sizeof(process_t *), &procs_len, &procs_cap, NULL, res.index)) {
        free(handle->argv);
        free(handle);
        mutex_release(NULL, &proc_mtx);
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return NULL;
    }
    procs[res.index] = handle;
    pid_counter++;

    // Add to the parent process' child list.
    if (parent) {
        mutex_acquire(NULL, &parent->mtx, TIMESTAMP_US_MAX);
        dlist_append(&parent->children, &handle->node);
        mutex_release(NULL, &parent->mtx);
    }

    // Initialise the empty memory map.
    memprotect_create(&handle->memmap.mpu_ctx);

    mutex_release(NULL, &proc_mtx);
    badge_err_set_ok(ec);
    return handle;
}

// Get a process handle by ID.
process_t *proc_get(pid_t pid) {
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    process_t *res = proc_get_unsafe(pid);
    mutex_release_shared(NULL, &proc_mtx);
    return res;
}

// Look up a process without locking the global process mutex.
process_t *proc_get_unsafe(pid_t pid) {
    process_t         dummy     = {.pid = pid};
    process_t        *dummy_ptr = &dummy;
    array_binsearch_t res       = array_binsearch(procs, sizeof(process_t *), procs_len, &dummy_ptr, proc_sort_pid_cmp);
    return res.found ? procs[res.index] : NULL;
}

// Get the process' flags.
uint32_t proc_getflags_raw(process_t *process) {
    return atomic_load(&process->flags);
}

// Get a handle to the current process, if any.
process_t *proc_current() {
    return sched_get_current_thread()->process;
}


// Set arguments for a process.
// If omitted, argc will be 0 and argv will be NULL.
static bool proc_setargs_raw_unsafe(badge_err_t *ec, process_t *process, int argc, char const *const *argv) {
    // Measure required memory for argv.
    size_t required = sizeof(size_t) * ((size_t)argc + 1);
    for (size_t i = 0; i < (size_t)argc; i++) {
        required += cstr_length(argv[i]) + 1;
    }

    // Allocate memory for the argv.
    char *mem = realloc(process->argv, required);
    if (!mem) {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return false;
    }

    // Copy datas into the argv.
    size_t off = sizeof(size_t) * ((size_t)argc + 1);
    for (size_t i = 0; i < (size_t)argc; i++) {
        // Argument pointer.
        *(char **)(mem + sizeof(size_t) * i) = (mem + off);

        // Copy in the string.
        size_t len = cstr_length(argv[i]);
        mem_copy(mem + off, argv[i], len + 1);
        off += len;
    }
    // Last argument pointer.
    *(char **)(mem + sizeof(size_t) * argc) = NULL;
    // Update argv size.
    process->argv_size                      = required;
    process->argv                           = (char **)mem;

    badge_err_set_ok(ec);
    return true;
}

// Load an executable and start a prepared process.
void proc_start_raw(badge_err_t *ec, process_t *process) {
    // Claim the process for starting.
    if (!(atomic_fetch_and(&process->flags, ~PROC_PRESTART) & PROC_PRESTART)) {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_STATE);
        return;
    }

    mutex_acquire(NULL, &process->mtx, TIMESTAMP_US_MAX);

    // Load the executable.
    kbelf_dyn dyn = kbelf_dyn_create(process->pid);
    if (!dyn) {
        logkf(LOG_ERROR, "Out of memory to start %{cs}", process->binary);
        mutex_release(NULL, &process->mtx);
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return;
    }
    if (!kbelf_dyn_set_exec(dyn, process->binary, NULL)) {
        logkf(LOG_ERROR, "Failed to open %{cs}", process->binary);
        kbelf_dyn_destroy(dyn);
        mutex_release(NULL, &process->mtx);
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
        return;
    }
    if (!kbelf_dyn_load(dyn)) {
        kbelf_dyn_destroy(dyn);
        logkf(LOG_ERROR, "Failed to load %{cs}", process->binary);
        mutex_release(NULL, &process->mtx);
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_FORMAT);
        return;
    }

    // Create the process' main thread.
    sched_thread_t *thread = proc_create_thread_raw_unsafe(
        ec,
        process,
        (sched_entry_point_t)kbelf_dyn_entrypoint(dyn),
        NULL,
        SCHED_PRIO_NORMAL
    );
    if (!thread) {
        kbelf_dyn_unload(dyn);
        kbelf_dyn_destroy(dyn);
        mutex_release(NULL, &process->mtx);
        return;
    }
    port_fencei();
    atomic_store(&process->flags, PROC_RUNNING);
    sched_resume_thread(ec, thread);
    mutex_release(NULL, &process->mtx);
    kbelf_dyn_destroy(dyn);
    logkf(LOG_INFO, "Process %{d} started", process->pid);
}


// Create a new thread in a process.
// Returns created thread handle.
sched_thread_t *proc_create_thread_raw_unsafe(
    badge_err_t *ec, process_t *process, sched_entry_point_t entry_point, void *arg, sched_prio_t priority
) {
    // Create an entry for a new thread.
    void *mem = realloc(process->threads, sizeof(sched_thread_t *) * (process->threads_len + 1));
    if (!mem) {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return NULL;
    }
    process->threads = mem;

    // TODO: Use a proper allocator for the kernel stack?
    size_t const kstack_size = 8192;
    void        *kstack      = malloc(kstack_size);
    if (!kstack) {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return NULL;
    }

    // Create a thread.
    sched_thread_t *thread = sched_create_userland_thread(ec, process, entry_point, arg, kstack, kstack_size, priority);
    if (!thread) {
        free(kstack);
        return NULL;
    }
    thread->user_isr_ctx.mpu_ctx   = &process->memmap.mpu_ctx;
    thread->kernel_isr_ctx.mpu_ctx = &process->memmap.mpu_ctx;

    // Add the thread to the list.
    array_insert(process->threads, sizeof(sched_thread_t *), process->threads_len, &thread, process->threads_len);
    process->threads_len++;
    // logkf(LOG_DEBUG, "Creating user thread, PC: 0x%{size;x}", entry_point);

    return thread;
}


// Delete a thread in a process.
void proc_delete_thread_raw_unsafe(badge_err_t *ec, process_t *process, sched_thread_t *thread) {
    (void)ec;
    (void)process;
    (void)thread;
    __builtin_trap();
}

// Memory map address comparator.
static int prog_memmap_cmp(void const *a, void const *b) {
    proc_memmap_ent_t const *a_ptr = a;
    proc_memmap_ent_t const *b_ptr = b;
    if (a_ptr->base < b_ptr->base)
        return -1;
    if (a_ptr->base < b_ptr->base)
        return 1;
    return 0;
}

// Sort the memory map by ascending address.
static inline void proc_memmap_sort_raw(proc_memmap_t *memmap) {
    array_sort(&memmap->regions[0], sizeof(memmap->regions[0]), memmap->regions_len, prog_memmap_cmp);
}

// Allocate more memory to a process.
size_t proc_map_raw(badge_err_t *ec, process_t *proc, size_t vaddr_req, size_t min_size, size_t min_align, int flags) {
    (void)min_align;
    (void)vaddr_req;

    proc_memmap_t *map = &proc->memmap;
    if (map->regions_len >= PROC_MEMMAP_MAX_REGIONS) {
        logk(LOG_WARN, "Out of regions");
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return 0;
    }

    // Allocate memory to the process.
    void *base = buddy_allocate(min_size, BLOCK_TYPE_USER, 0);
    if (!base) {
        logk(LOG_WARN, "Out of vmem");
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return 0;
    }
    size_t size = buddy_get_size(base);
    mem_set(base, 0, size);

    // Account the process's memory.
    map->regions[map->regions_len] = (proc_memmap_ent_t){
        .base  = (size_t)base,
        .size  = size,
        .write = true,
        .exec  = true,
    };
    map->regions_len++;
    proc_memmap_sort_raw(map);

    // Update memory protection.
    if (!memprotect(map, &map->mpu_ctx, (size_t)base, (size_t)base, size, flags & MEMPROTECT_FLAG_RWX)) {
        for (size_t i = 0; i < map->regions_len; i++) {
            if (map->regions[i].base == (size_t)base) {
                array_remove(&map->regions[0], sizeof(map->regions[0]), map->regions_len, NULL, i);
                break;
            }
        }
        map->regions_len--;
        buddy_deallocate(base);
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return 0;
    }
    memprotect_commit(&map->mpu_ctx);

    logkf(LOG_INFO, "Mapped %{size;d} bytes at %{size;x} to process %{d}", size, base, proc->pid);
    badge_err_set_ok(ec);
    return (size_t)base;
}

// Release memory allocated to a process.
void proc_unmap_raw(badge_err_t *ec, process_t *proc, size_t base) {
    proc_memmap_t *map = &proc->memmap;
    for (size_t i = 0; i < map->regions_len; i++) {
        if (map->regions[i].base == base) {
            proc_memmap_ent_t region = map->regions[i];
            array_remove(&map->regions[0], sizeof(map->regions[0]), map->regions_len, NULL, i);
            map->regions_len--;
            assert_dev_keep(memprotect(map, &map->mpu_ctx, base, base, region.size, 0));
            memprotect_commit(&map->mpu_ctx);
            buddy_deallocate((void *)base);
            badge_err_set_ok(ec);
            logkf(LOG_INFO, "Unmapped %{size;d} bytes at %{size;x} from process %{d}", region.size, base, proc->pid);
            return;
        }
    }
    badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
}

// Whether the process owns this range of memory.
// Returns the lowest common denominator of the access bits bitwise or 8.
int proc_map_contains_raw(process_t *proc, size_t base, size_t size) {
    // Align to whole pages.
    if (base % MEMMAP_PAGE_SIZE) {
        size += base % MEMMAP_PAGE_SIZE;
        base -= base % MEMMAP_PAGE_SIZE;
    }
    if (size % MEMMAP_PAGE_SIZE) {
        size += MEMMAP_PAGE_SIZE - size % MEMMAP_PAGE_SIZE;
    }

    mutex_acquire_shared(NULL, &proc->mtx, TIMESTAMP_US_MAX);
    int access = 7;
    while (size) {
        size_t i;
        for (i = 0; i < proc->memmap.regions_len; i++) {
            if (base >= proc->memmap.regions[i].base &&
                base < proc->memmap.regions[i].base + proc->memmap.regions[i].size) {
                goto found;
            }
        }

        // This page is not in the region map.
        mutex_release_shared(NULL, &proc->mtx);
        return 0;

    found:
        // This page is in the region map.
        if (proc->memmap.regions[i].size > size) {
            // All pages found.
            break;
        }
        base += proc->memmap.regions[i].size;
        size += proc->memmap.regions[i].size;
    }
    mutex_release_shared(NULL, &proc->mtx);
    return access | 8;
}

// Add a file to the process file handle list.
int proc_add_fd_raw(badge_err_t *ec, process_t *process, file_t real) {
    proc_fd_t fd = {.real = real, .virt = 0};
    for (size_t i = 0; i < process->fds_len; i++) {
        if (process->fds[i].virt > fd.virt) {
            fd.virt = process->fds[i].virt + 1;
        }
    }
    if (array_len_insert(&process->fds, sizeof(proc_fd_t), &process->fds_len, &fd, process->fds_len)) {
        badge_err_set_ok(ec);
        return fd.virt;
    } else {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return -1;
    }
}

// Find a file in the process file handle list.
file_t proc_find_fd_raw(badge_err_t *ec, process_t *process, int virt) {
    for (size_t i = 0; i < process->fds_len; i++) {
        if (process->fds[i].virt == virt) {
            badge_err_set_ok(ec);
            return process->fds[i].real;
        }
    }
    badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
    return -1;
}

// Remove a file from the process file handle list.
void proc_remove_fd_raw(badge_err_t *ec, process_t *process, int virt) {
    for (size_t i = 0; i < process->fds_len; i++) {
        if (process->fds[i].virt == virt) {
            array_len_remove(&process->fds, sizeof(proc_fd_t), &process->fds_len, NULL, i);
            badge_err_set_ok(ec);
            return;
        }
    }
    badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
}


// Perform a pre-resume check for a user thread.
// Used to implement asynchronous events.
void proc_pre_resume_cb(sched_thread_t *thread) {
    process_t *const process = thread->process;
    if (proc_signals_pending_raw(process)) {
        logk_from_isr(LOG_DEBUG, "There be pending signals");
        sched_raise_from_isr(thread, false, proc_signal_handler);
    }
}

// Atomically check for pending signals.
bool proc_signals_pending_raw(process_t *process) {
    return atomic_load(&process->flags) & PROC_SIGPEND;
}

// Raise a signal to a process' main thread or a specified thread, while suspending it's other threads.
void proc_raise_signal_raw(badge_err_t *ec, process_t *process, int signum) {
    mutex_acquire(NULL, &process->mtx, TIMESTAMP_US_MAX);
    sigpending_t *node = malloc(sizeof(sigpending_t));
    if (!node) {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
    } else {
        node->signum = signum;
        dlist_append(&process->sigpending, &node->node);
        atomic_fetch_or(&process->flags, PROC_SIGPEND);
    }
    mutex_release(NULL, &process->mtx);
}



// Suspend all threads for a process except the current.
void proc_suspend(process_t *process, sched_thread_t *current) {
    mutex_acquire(NULL, &process->mtx, TIMESTAMP_US_MAX);
    for (size_t i = 0; i < process->threads_len; i++) {
        if (process->threads[i] != current) {
            sched_suspend_thread(NULL, process->threads[i]);
        }
    }
    mutex_release(NULL, &process->mtx);
}

// Resume all threads for a process.
void proc_resume(process_t *process) {
    mutex_acquire(NULL, &process->mtx, TIMESTAMP_US_MAX);
    for (size_t i = 0; i < process->threads_len; i++) {
        sched_resume_thread(NULL, process->threads[i]);
    }
    mutex_release(NULL, &process->mtx);
}

// Release all process runtime resources (threads, memory, files, etc.).
// Does not remove args, exit code, etc.
void proc_delete_runtime_raw(process_t *process) {
    // This may not be run from one of the process' threads because it kills all of them.
    for (size_t i = 0; i < process->threads_len; i++) {
        assert_dev_drop(sched_get_current_thread() != process->threads[i]);
    }

    if (process->pid == 1 && !allow_proc1_death()) {
        // Process 1 exited and now the kernel is dead.
        logkf(LOG_FATAL, "Process 1 exited unexpectedly");
        panic_abort();
    }

    // Set the exiting flag so any return to user-mode kills the thread.
    mutex_acquire(NULL, &process->mtx, TIMESTAMP_US_MAX);
    if (atomic_load(&process->flags) & PROC_EXITED) {
        // Already exited, return now.
        mutex_release(NULL, &process->mtx);
        return;
    } else {
        // Flag the scheduler to start suspending threads.
        atomic_fetch_or(&process->flags, PROC_EXITING);
        atomic_thread_fence(memory_order_release);
    }

    // Wait for the scheduler to suspend all the threads.
    for (size_t i = 0; i < process->threads_len; i++) {
        sched_resume_thread(NULL, process->threads[i]);
    }
    bool waiting = true;
    while (waiting) {
        waiting = false;
        for (size_t i = 0; i < process->threads_len; i++) {
            if (sched_thread_is_running(NULL, process->threads[i])) {
                waiting = true;
            }
        }
        sched_yield();
    }

    // Adopt all children to init.
    if (process->pid != 1) {
        process_t *init = procs[0];
        assert_dev_drop(init->pid == 1);
        dlist_concat(&init->children, &process->children);
    }

    // Destroy all threads.
    for (size_t i = 0; i < process->threads_len; i++) {
        free((void *)process->threads[i]->kernel_stack_bottom);
        sched_destroy_thread(NULL, process->threads[i]);
    }
    process->threads_len = 0;
    free(process->threads);

    // Unmap all memory regions.
    while (process->memmap.regions_len) {
        proc_unmap_raw(NULL, process, process->memmap.regions[0].base);
    }

    // TODO: Close pipes and files.

    // Mark the process as exited.
    process->flags |= PROC_EXITED;
    process->flags &= ~PROC_EXITING & ~PROC_RUNNING;
    logkf(LOG_INFO, "Process %{d} stopped with code %{d}", process->pid, process->state_code);
    mutex_release(NULL, &process->mtx);
}



// Create a new, empty process.
pid_t proc_create(badge_err_t *ec, pid_t parent, char const *binary, int argc, char const *const *argv) {
    process_t *process = proc_create_raw(ec, parent, binary, argc, argv);
    return process ? process->pid : -1;
}

// Delete a process and release any resources it had.
static bool proc_delete_impl(pid_t pid, bool only_prestart) {
    mutex_acquire(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    process_t         dummy     = {.pid = pid};
    process_t        *dummy_ptr = &dummy;
    array_binsearch_t res       = array_binsearch(procs, sizeof(process_t *), procs_len, &dummy_ptr, proc_sort_pid_cmp);
    if (!res.found) {
        mutex_release(NULL, &proc_mtx);
        return false;
    }
    process_t *handle = procs[res.index];

    // Check for pre-start-ness.
    if (only_prestart && !(atomic_load(&handle->flags) & PROC_PRESTART)) {
        mutex_release(NULL, &proc_mtx);
        return false;
    }

    // Stop the possibly running process and release all run-time resources.
    proc_delete_runtime_raw(handle);

    // Remove from parent's child list.
    process_t *parent = handle->parent;
    if (parent) {
        mutex_acquire(NULL, &parent->mtx, TIMESTAMP_US_MAX);
        dlist_remove(&parent->children, &handle->node);
        mutex_release(NULL, &parent->mtx);
    }

    // Release kernel memory allocated to process.
    memprotect_destroy(&handle->memmap.mpu_ctx);
    free(handle->argv);
    free(handle);
    array_lencap_remove(&procs, sizeof(process_t *), &procs_len, &procs_cap, NULL, res.index);
    mutex_release(NULL, &proc_mtx);

    return true;
}

// Delete a process only if it hasn't been started yet.
bool proc_delete_prestart(pid_t pid) {
    return proc_delete_impl(pid, true);
}

// Delete a process and release any resources it had.
void proc_delete(pid_t pid) {
    proc_delete_impl(pid, false);
}

// Get the process' flags.
uint32_t proc_getflags(badge_err_t *ec, pid_t pid) {
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    process_t *proc = proc_get(pid);
    uint32_t   flags;
    if (proc) {
        flags = atomic_load(&proc->flags);
        badge_err_set_ok(ec);
    } else {
        flags = 0;
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
    }
    mutex_release_shared(NULL, &proc_mtx);
    return flags;
}


// Load an executable and start a prepared process.
void proc_start(badge_err_t *ec, pid_t pid) {
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    process_t *proc = proc_get_unsafe(pid);
    if (proc) {
        proc_start_raw(ec, proc);
    } else {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
    }
    mutex_release_shared(NULL, &proc_mtx);
}


// Allocate more memory to a process.
// Returns actual virtual address on success, 0 on failure.
size_t proc_map(badge_err_t *ec, pid_t pid, size_t vaddr_req, size_t min_size, size_t min_align, int flags) {
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    process_t *proc = proc_get(pid);
    size_t     res  = 0;
    if (proc) {
        res = proc_map_raw(ec, proc, vaddr_req, min_size, min_align, flags);
    } else {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
    }
    mutex_release_shared(NULL, &proc_mtx);
    return res;
}

// Release memory allocated to a process.
void proc_unmap(badge_err_t *ec, pid_t pid, size_t base) {
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    process_t *proc = proc_get(pid);
    if (proc) {
        proc_unmap_raw(ec, proc, base);
    } else {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
    }
    mutex_release_shared(NULL, &proc_mtx);
}

// Whether the process owns this range of memory.
// Returns the lowest common denominator of the access bits bitwise or 8.
int proc_map_contains(badge_err_t *ec, pid_t pid, size_t base, size_t size) {
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    process_t *proc = proc_get(pid);
    int        ret  = 0;
    if (proc) {
        ret = proc_map_contains_raw(proc, base, size);
        badge_err_set_ok(ec);
    } else {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
    }
    mutex_release_shared(NULL, &proc_mtx);
    return ret;
}

// Check whether a process is a parent to another.
bool proc_is_parent(pid_t parent, pid_t child) {
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    process_t *parent_proc = proc_get_unsafe(parent);
    process_t *child_proc  = proc_get_unsafe(child);
    bool       eq          = child_proc && child_proc->parent == parent_proc;
    mutex_release_shared(NULL, &proc_mtx);
    return eq;
}


// Raise a signal to a process' main thread, while suspending it's other threads.
void proc_raise_signal(badge_err_t *ec, pid_t pid, int signum) {
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    process_t *proc = proc_get(pid);
    if (proc) {
        proc_raise_signal_raw(ec, proc, signum);
    } else {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
    }
    mutex_release_shared(NULL, &proc_mtx);
}



// Determine string length in memory a user owns.
// Returns -1 if the user doesn't have access to any byte in the string.
ptrdiff_t strlen_from_user(pid_t pid, size_t user_vaddr, ptrdiff_t max_len) {
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    ptrdiff_t res = strlen_from_user_raw(proc_get_unsafe(pid), user_vaddr, max_len);
    mutex_release_shared(NULL, &proc_mtx);
    return res;
}

// Copy bytes from user to kernel.
// Returns whether the user has access to all of these bytes.
// If the user doesn't have access, no copy is performed.
bool copy_from_user(pid_t pid, void *kernel_vaddr, size_t user_vaddr, size_t len) {
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    bool res = copy_from_user_raw(proc_get_unsafe(pid), kernel_vaddr, user_vaddr, len);
    mutex_release_shared(NULL, &proc_mtx);
    return res;
}

// Copy from kernel to user.
// Returns whether the user has access to all of these bytes.
// If the user doesn't have access, no copy is performed.
bool copy_to_user(pid_t pid, size_t user_vaddr, void *kernel_vaddr, size_t len) {
    mutex_acquire_shared(NULL, &proc_mtx, TIMESTAMP_US_MAX);
    bool res = copy_to_user_raw(proc_get_unsafe(pid), user_vaddr, kernel_vaddr, len);
    mutex_release_shared(NULL, &proc_mtx);
    return res;
}
