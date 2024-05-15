
// SPDX-License-Identifier: MIT

#include "usercopy.h"

#include "badge_strings.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "process/internal.h"



// Determine string length in memory a user owns.
// Returns -1 if the user doesn't have access to any byte in the string.
ptrdiff_t strlen_from_user_raw(process_t *process, size_t user_vaddr, ptrdiff_t max_len) {
    if (!max_len) {
        return 0;
    }
    ptrdiff_t len = 0;
    mutex_acquire_shared(NULL, &process->mtx, TIMESTAMP_US_MAX);

    // Check first page permissions.
    if (!(proc_map_contains_raw(process, user_vaddr, 1) & MEMPROTECT_FLAG_R)) {
        mutex_release_shared(NULL, &process->mtx);
        return -1;
    }

    // String length loop.
    while (len < max_len && *(char const *)user_vaddr) {
        len++;
        user_vaddr++;
        if (user_vaddr % MEMMAP_PAGE_SIZE == 0) {
            // Check further page permissions.
            if (!(proc_map_contains_raw(process, user_vaddr, 1) & MEMPROTECT_FLAG_R)) {
                mutex_release_shared(NULL, &process->mtx);
                return -1;
            }
        }
    }

    mutex_release_shared(NULL, &process->mtx);
    return len;
}

// Copy bytes from user to kernel.
// Returns whether the user has access to all of these bytes.
// If the user doesn't have access, no copy is performed.
bool copy_from_user_raw(process_t *process, void *kernel_vaddr, size_t user_vaddr, size_t len) {
    if (!proc_map_contains_raw(process, user_vaddr, len)) {
        return false;
    }
    mem_copy(kernel_vaddr, (void const *)user_vaddr, len);
    return true;
}

// Copy from kernel to user.
// Returns whether the user has access to all of these bytes.
// If the user doesn't have access, no copy is performed.
bool copy_to_user_raw(process_t *process, size_t user_vaddr, void *kernel_vaddr, size_t len) {
    if (!proc_map_contains_raw(process, user_vaddr, len)) {
        return false;
    }
    mem_copy((void *)user_vaddr, kernel_vaddr, len);
    return true;
}
