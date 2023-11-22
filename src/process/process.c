
// SPDX-License-Identifier: MIT

#include "process/process.h"

#include "arrays.h"
#include "badge_strings.h"
#include "isr_ctx.h"
#include "kbelf.h"
#include "log.h"
#include "malloc.h"
#include "port/process/process.h"
#include "process/internal.h"
#include "process/types.h"

#include <stdatomic.h>

// Globally unique PID number counter.
atomic_int pid_counter = 1;

process_t dummy_proc;



// Create a new, empty process.
process_t *proc_create(badge_err_t *ec) {
    (void)ec;
    // TODO: Allocate these instead of just handing a dummy out.
    process_t *handle = &dummy_proc;

    // Get a new PID.
    pid_t pid = atomic_fetch_add_explicit(&pid_counter, 1, memory_order_acquire);
    assert_always(pid >= 1);

    // Install default values.
    *handle = (process_t){
        .argc        = 0,
        .argv        = NULL,
        .threads_len = 0,
        .threads     = NULL,
        .memmap =
            {
                .regions_len = 0,
            },
        .mtx = MUTEX_T_INIT_SHARED,
        .pid = pid,
    };

    // Initialise the empty memory map.
    assert_dev_drop(proc_mpu_gen(&handle->memmap));

    return handle;
}

// Delete a process and release any resources it had.
void proc_delete(badge_err_t *ec, pid_t pid) {
    // TODO: Convert to atomic lookup and remove.
    process_t *handle = proc_get(ec, pid);
    if (!handle)
        return;

    // Stop the possibly running process and release all run-time resources.
    proc_delete_runtime(handle);

    // Release kernel memory allocated to process.
    free(handle->argv);
    badge_err_set_ok(ec);
}

// Get a process handle by ID.
process_t *proc_get(badge_err_t *ec, pid_t pid) {
    // TODO: Allocate these instead of just handing a dummy out.
    (void)ec;
    (void)pid;
    return &dummy_proc;
}

// Get the process' flags.
uint32_t proc_getflags(process_t *process) {
    return process->flags;
}


// Set arguments for a process.
// If omitted, argc will be 0 and argv will be NULL.
void proc_setargs(badge_err_t *ec, process_t *process, int argc, char const *const *argv) {
    // Measure required memory for argv.
    int required = sizeof(size_t) * argc;
    for (int i = 0; i < argc; i++) {
        required += cstr_length(argv[i]) + 1;
    }

    // Allocate memory for the argv.
    char *mem = realloc(process->argv, required);
    if (!mem) {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return;
    }

    // Copy datas into the argv.
    int off = sizeof(size_t) * argc;
    for (int i = 0; i < argc; i++) {
        // Argument pointer.
        *(char **)(mem + sizeof(size_t) * i) = (mem + off);

        // Copy in the string.
        size_t len = cstr_length(argv[i]);
        mem_copy(mem + off, argv[i], len + 1);
        off += len;
    }
    badge_err_set_ok(ec);
}

// Load an executable and start a prepared process.
void proc_start(badge_err_t *ec, process_t *process, char const *executable) {
    // Load the executable.
    logk(LOG_DEBUG, "Creating dynamic loader");
    kbelf_dyn dyn = kbelf_dyn_create(process->pid);
    logk(LOG_DEBUG, "Adding executable");
    if (!kbelf_dyn_set_exec(dyn, executable, NULL)) {
        kbelf_dyn_destroy(dyn);
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_UNKNOWN);
        return;
    }
    logk(LOG_DEBUG, "Loading program");
    if (!kbelf_dyn_load(dyn)) {
        kbelf_dyn_destroy(dyn);
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_UNKNOWN);
        return;
    }

    // Create the process' main thread.
    logk(LOG_DEBUG, "Creating main thread");
    sched_thread_t *thread =
        proc_create_thread(ec, process, (sched_entry_point_t)kbelf_dyn_entrypoint(dyn), NULL, SCHED_PRIO_NORMAL);
    if (!thread) {
        kbelf_dyn_unload(dyn);
        kbelf_dyn_destroy(dyn);
        return;
    }
    logk(LOG_DEBUG, "Starting main thread");
    process->flags = PROC_RUNNING;
    sched_resume_thread(ec, thread);
}


#define kstack_size 8192
char kstack[kstack_size] ALIGNED_TO(STACK_ALIGNMENT);

// Create a new thread in a process.
// Returns created thread handle.
sched_thread_t *proc_create_thread(
    badge_err_t *ec, process_t *process, sched_entry_point_t entry_point, void *arg, sched_prio_t priority
) {
    // TODO: Locking for process threads.
    // Create an entry for a new thread.
    void *mem = realloc(process->threads, sizeof(*process->threads) * (process->threads_len + 1));
    if (!mem) {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return NULL;
    }
    process->threads = mem;

    // TODO: Use a proper allocator for the kernel stack.
    // size_t const kstack_size = 8192;
    // void        *kstack      = malloc(kstack_size);
    // if (!kstack) {
    //     badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
    //     return NULL;
    // }

    // Create a thread.
    sched_thread_t *thread = sched_create_userland_thread(ec, process, entry_point, arg, kstack, kstack_size, priority);
    if (!thread) {
        // free(kstack);
        return NULL;
    }

    // Add the thread to the list.
    array_insert(process->threads, sizeof(*process->threads), process->threads_len, &thread, process->threads_len);
    process->threads_len++;
    logkf(LOG_DEBUG, "Creating user thread, PC: 0x%{size;x}", entry_point);

    return thread;
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
static inline void proc_memmap_sort(proc_memmap_t *memmap) {
    array_sort(&memmap->regions[0], sizeof(memmap->regions[0]), memmap->regions_len, prog_memmap_cmp);
}

// Allocate more memory to a process.
size_t proc_map(badge_err_t *ec, process_t *proc, size_t vaddr_req, size_t min_size, size_t min_align) {
    (void)min_align;
    (void)vaddr_req;

    proc_memmap_t *map = &proc->memmap;
    if (map->regions_len >= PROC_MEMMAP_MAX_REGIONS) {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return 0;
    }

    void *base = malloc(min_size);
    if (!base) {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        return 0;
    }

    map->regions[map->regions_len] = (proc_memmap_ent_t){
        .base  = (size_t)base,
        .size  = (size_t)min_size,
        .write = true,
        .exec  = true,
    };
    map->regions_len++;
    proc_memmap_sort(map);
    if (!proc_mpu_gen(map)) {
        badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOMEM);
        proc_unmap(NULL, proc, (size_t)base);
        return 0;
    }

    logkf(LOG_INFO, "Mapped %{size;d} bytes at %{size;x} to process %{d}", min_size, base, proc->pid);
    badge_err_set_ok(ec);
    return (size_t)base;
}

// Release memory allocated to a process.
void proc_unmap(badge_err_t *ec, process_t *proc, size_t base) {
    proc_memmap_t *map = &proc->memmap;
    for (size_t i = 0; i < map->regions_len; i++) {
        if (map->regions[i].base == base) {
            free((void *)base);
            array_remove(&map->regions[0], sizeof(map->regions[0]), map->regions_len, NULL, i);
            map->regions_len--;
            badge_err_set_ok(ec);
            return;
        }
    }
    badge_err_set(ec, ELOC_PROCESS, ECAUSE_NOTFOUND);
}



// Suspend all threads for a process except the current.
void proc_suspend(process_t *process, sched_thread_t *current) {
    assert_always(mutex_acquire(NULL, &process->mtx, PROC_MTX_TIMEOUT));
    for (size_t i = 0; i < process->threads_len; i++) {
        if (process->threads[i] != current) {
            sched_suspend_thread(NULL, process->threads[i]);
        }
    }
    mutex_release(NULL, &process->mtx);
}

// Resume all threads for a process.
void proc_resume(process_t *process) {
    assert_always(mutex_acquire(NULL, &process->mtx, PROC_MTX_TIMEOUT));
    for (size_t i = 0; i < process->threads_len; i++) {
        sched_resume_thread(NULL, process->threads[i]);
    }
    mutex_release(NULL, &process->mtx);
}


// Release all process runtime resources (threads, memory, files, etc.).
// Does not remove args, exit code, etc.
void proc_delete_runtime(process_t *process) {
    // This may not be run from one of the process' threads because it kills all of them.
    for (size_t i = 0; i < process->threads_len; i++) {
        assert_dev_drop(sched_get_current_thread() != process->threads[i]);
    }

    // Set the exiting flag so any return to user-mode kills the thread.
    assert_always(mutex_acquire(NULL, &process->mtx, PROC_MTX_TIMEOUT));
    if (process->flags & (PROC_EXITED | PROC_EXITING)) {
        // Already exiting or exited, return now.
        return;
        mutex_release(NULL, &process->mtx);
    } else {
        // Flag the scheduler to start suspending threads.
        process->flags |= PROC_EXITING;
        atomic_thread_fence(memory_order_release);
    }

    // Wait for the scheduler to suspend all the threads.
    proc_resume(process);
    bool waiting = true;
    while (waiting) {
        waiting = false;
        for (size_t i = 0; i < process->threads_len; i++) {
            if (sched_thread_is_running(NULL, process->threads[i]))
                waiting = true;
        }
        sched_yield();
    }

    // Destroy all threads.
    // TODO: Release kernel stacks.
    for (size_t i = 0; i < process->threads_len; i++) {
        sched_destroy_thread(NULL, process->threads[i]);
    }
    process->threads_len = 0;
    free(process->threads);

    // Unmap all memory regions.
    while (process->memmap.regions_len) {
        proc_unmap(NULL, process, process->memmap.regions[0].base);
    }

    // TODO: Close pipes and files.

    // Mark the process as exited.
    process->flags |= PROC_EXITED;
    process->flags &= ~PROC_EXITING & ~PROC_RUNNING;
    mutex_release(NULL, &process->mtx);
}
