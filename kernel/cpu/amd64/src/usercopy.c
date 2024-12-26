
// SPDX-License-Identifier: MIT

#include "usercopy.h"

#include "assertions.h"
#include "badge_strings.h"
#include "cpu/mmu.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "memprotect.h"
#include "process/internal.h"

// TODO: Convert to page fault intercepting memcpy.
// TODO: Migrate into generic process API.



// Determine string length in memory a user owns.
// Returns -1 if the user doesn't have access to any byte in the string.
ptrdiff_t strlen_from_user_raw(process_t *process, size_t user_vaddr, ptrdiff_t max_len) {
    // TODO.
    return -1;
}

// Copy bytes from user to kernel.
// Returns whether the user has access to all of these bytes.
// If the user doesn't have access, no copy is performed.
bool copy_from_user_raw(process_t *process, void *kernel_vaddr, size_t user_vaddr, size_t len) {
    // TODO.
    return false;
}

// Copy from kernel to user.
// Returns whether the user has access to all of these bytes.
// If the user doesn't have access, no copy is performed.
bool copy_to_user_raw(process_t *process, size_t user_vaddr, void *kernel_vaddr0, size_t len) {
    // TODO.
    return false;
}
