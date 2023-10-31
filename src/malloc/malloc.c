
// SPDX-License-Identifier: MIT

#ifndef BADGEOS_KERNEL
#define _GNU_SOURCE
#endif

#include "debug.h"
#include "spinlock.h"

#include <stddef.h>
#include <stdint.h>

#ifdef BADGEROS_KERNEL
extern char __start_free_sram[];
extern char __stop_free_sram[];

#define __wrap_malloc         malloc
#define __wrap_free           free
#define __wrap_calloc         calloc
#define __wrap_realloc        realloc
#define __wrap_reallocarray   reallocarray
#define __wrap_aligned_alloc  aligned_alloc
#define __wrap_posix_memalign posix_memalign

#else
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

void *__real_malloc(size_t size);
void  __real_free(void *ptr);
void *__real_calloc(size_t nmemb, size_t size);
void *__real_realloc(void *ptr, size_t size);
void *__real_reallocarray(void *ptr, size_t nmemb, size_t size);
#endif

#ifdef PRELOAD
#define __wrap_malloc         malloc
#define __wrap_free           free
#define __wrap_calloc         calloc
#define __wrap_realloc        realloc
#define __wrap_reallocarray   reallocarray
#define __wrap_aligned_alloc  aligned_alloc
#define __wrap_posix_memalign posix_memalign
#endif

#define ALIGNMENT   8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

#define ALIGN_UP(x, y)   (void *)(((size_t)(x) + (y - 1)) & ~(y - 1))
#define ALIGN_DOWN(x, y) (void *)((size_t)(x) & ~(y - 1))

#define NUM_SIZE_CLASSES 6
#define MBLK_SIZE        ALIGN(sizeof(free_blk_header_t))

typedef struct free_blk_header {
    size_t                  size;
    struct free_blk_header *next;
    struct free_blk_header *prior;
} free_blk_header_t;

static char              *mem_start;
static char              *mem_end;
static char              *mem_end_max;
static free_blk_header_t *free_lists = NULL;
static atomic_flag        lock       = ATOMIC_FLAG_INIT;

void         kernel_heap_init();
static void  try_split(free_blk_header_t *fp, size_t needed);
static void *find_fit(size_t size);

size_t min_class_size[] = {MBLK_SIZE, 64, 128, 256, 1024, 4096};

#ifndef BADGEROS_KERNEL
void print_heap() {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        printf("Bucket size: " FMT_ZI "\n", min_class_size[i]);
        free_blk_header_t *fp = &free_lists[i];

        while (fp->next != &free_lists[i]) {
            printf("\tFree block of size: " FMT_ZI "\n", fp->next->size);
            fp = fp->next;
        }
    }
}
#else

void print_heap() {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        logkf(LOG_DEBUG, "Bucket size: %{size;d}", min_class_size[i]);
        free_blk_header_t *fp = &free_lists[i];

        while (fp->next != &free_lists[i]) {
            logkf(LOG_DEBUG, "\tFree block 0x%{size;x} of size: %{size;d}", fp->next, fp->next->size);
            fp = fp->next;
        }
    }
}
#endif

void kernel_heap_init() {
#ifdef BADGEROS_KERNEL
    free_lists  = (free_blk_header_t *)__start_free_sram;
    mem_end_max = __stop_free_sram;
#else
    SPIN_LOCK_LOCK(lock);
    if (free_lists) {
        SPIN_LOCK_UNLOCK(lock);
        return;
    }

    sbrk(1024 * 1024); // buffer
    free_lists  = sbrk(1024 * 1024 * 1024);
    mem_end_max = sbrk(0);
    BADGEROS_MALLOC_ASSERT_DEBUG(free_lists != (void *)-1, "sbrk() failed");
#endif

    mem_start = ((char *)free_lists) + (NUM_SIZE_CLASSES * sizeof(free_blk_header_t));
    mem_end   = mem_start;

    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        free_lists[i].size = 0;
        free_lists[i].next = free_lists[i].prior = &free_lists[i];
    }

#ifndef BADGEROS_KERNEL
    SPIN_LOCK_UNLOCK(lock);
#endif
}

static void *find_fit(size_t size) {
    free_blk_header_t *fp;

    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        BADGEROS_MALLOC_ASSERT_DEBUG(
            ((void *)free_lists[i].next >= (void *)free_lists && (void *)free_lists[i].next < (void *)mem_end_max),
            "find_fit: corrupted linked list free_lists[" FMT_I "].next = " FMT_P " valid range: " FMT_P "-" FMT_P,
            i,
            free_lists[i].next,
            free_lists,
            mem_end_max
        );
        if (min_class_size[i] >= size && free_lists[i].next != &free_lists[i]) {
            fp                 = free_lists[i].next;
            free_lists[i].next = fp->next;
            fp->next->prior    = &free_lists[i];
            try_split(fp, size);
            return fp;
        }
    }

    fp = &free_lists[NUM_SIZE_CLASSES - 1];
    while (fp->next != &free_lists[NUM_SIZE_CLASSES - 1]) {
        if (fp->size >= size) {
            fp->next->prior = fp->prior;
            fp->prior->next = fp->next;
            try_split(fp, size);
            return fp;
        }
        fp = fp->next;
    }

    return NULL;
}

static void try_split(free_blk_header_t *fp, size_t needed) {
    if (fp->size < needed)
        return;
    size_t             remaining = fp->size - needed;
    free_blk_header_t *sp;

    if (remaining < MBLK_SIZE)
        return;

    fp->size = needed;
    sp       = (free_blk_header_t *)((char *)fp + needed);
    sp->size = remaining;

    for (int i = NUM_SIZE_CLASSES - 1; i > 0; i--) {
        BADGEROS_MALLOC_ASSERT_DEBUG(
            ((void *)free_lists[i].next >= (void *)free_lists && (void *)free_lists[i].next < (void *)mem_end_max),
            "try_split: corrupted linked list free_lists[" FMT_I "].next = " FMT_P " valid range: " FMT_P "-" FMT_P,
            i,
            free_lists[i].next,
            free_lists,
            mem_end_max
        );
        if (min_class_size[i] <= remaining) {
            sp->prior          = &free_lists[i];
            sp->next           = free_lists[i].next;
            free_lists[i].next = free_lists[i].next->prior = sp;
            break;
        }
    }
}

void *_malloc(size_t size) {
    if (!size)
        size = 1;

    size_t *header;
    size_t  blk_size = ALIGN(size + sizeof(size_t));

    blk_size = (blk_size < MBLK_SIZE) ? MBLK_SIZE : blk_size;
    header   = find_fit(blk_size);

    if (header) {
        *header = *(size_t *)header | 1;
    } else {
        header = ALIGN_UP((size_t *)mem_end, 8);

        if ((void *)header > (void *)mem_end_max) {
            BADGEROS_MALLOC_MSG_DEBUG("malloc: out of memory, returning NULL");
            return NULL;
        }

        mem_end = (char *)header + blk_size;
        *header = blk_size | 1;
    }
    void *ptr = (char *)header + sizeof(size_t);
    BADGEROS_MALLOC_ASSERT_DEBUG(
        (ptr >= (void *)mem_start && ptr < (void *)mem_end_max),
        "malloc: invalid pointer " FMT_P " range " FMT_P "-" FMT_P,
        ptr,
        mem_start,
        mem_end_max
    );
    return ptr;
}

void *__wrap_malloc(size_t size) {
#ifdef PRELOAD
    if (!free_lists)
        kernel_heap_init();
#endif
    SPIN_LOCK_LOCK(lock);
    void *ptr = _malloc(size);
    SPIN_LOCK_UNLOCK(lock);

    return ptr;
}

void *__wrap_aligned_alloc(size_t alignment, size_t size) {
    (void)alignment;
#ifdef PRELOAD
    if (!free_lists)
        kernel_heap_init();
#endif
    SPIN_LOCK_LOCK(lock);
    void *ptr = _malloc(size);
    SPIN_LOCK_UNLOCK(lock);

    return ptr;
}

int __wrap_posix_memalign(void **memptr, size_t alignment, size_t size) {
    (void)alignment;
#ifdef PRELOAD
    if (!free_lists)
        kernel_heap_init();
#endif
    SPIN_LOCK_LOCK(lock);
    void *ptr = _malloc(size);
    SPIN_LOCK_UNLOCK(lock);

    *memptr = ptr;
    return 0;
}

void *__wrap_calloc(size_t nmemb, size_t size) {
#ifdef PRELOAD
    if (!free_lists)
        kernel_heap_init();
#endif

    SPIN_LOCK_LOCK(lock);
    void *ptr = _malloc(nmemb * size);
    if (ptr)
        __builtin_memset(ptr, 0, nmemb * size);
    SPIN_LOCK_UNLOCK(lock);
    return ptr;
}

static void _free(void *ptr) {
    if (!ptr) {
        return;
    }

    BADGEROS_MALLOC_ASSERT_DEBUG(
        (ptr >= (void *)mem_start && ptr < (void *)mem_end_max),
        "free: invalid pointer " FMT_P " range " FMT_P "-" FMT_P,
        ptr,
        mem_start,
        mem_end_max
    );

    free_blk_header_t *header = (free_blk_header_t *)((char *)ptr - sizeof(size_t));
    size_t             size   = header->size;
    header->size              = *(size_t *)header & ~1L;
    BADGEROS_MALLOC_ASSERT_DEBUG(size & 1, "free: double free on pointer " FMT_P, ptr);

    for (int i = NUM_SIZE_CLASSES - 1; i >= 0; i--) {
        if (min_class_size[i] <= size) {
            header->next       = free_lists[i].next;
            header->prior      = &free_lists[i];
            free_lists[i].next = free_lists[i].next->prior = header;
            return;
        }
    }
}

void __wrap_free(void *ptr) {
#ifdef PRELOAD
    if (!free_lists)
        kernel_heap_init();
#endif

    SPIN_LOCK_LOCK(lock);
    _free(ptr);
    SPIN_LOCK_UNLOCK(lock);
}

void *__wrap_realloc(void *ptr, size_t size) {
#ifdef PRELOAD
    if (!free_lists)
        kernel_heap_init();
#endif

    if (!ptr) {
        return __wrap_malloc(size);
    }

    if (!size) {
        __wrap_free(ptr);
        return NULL;
    }

    BADGEROS_MALLOC_ASSERT_DEBUG(
        (ptr >= (void *)mem_start && ptr < (void *)mem_end_max),
        "realloc: invalid pointer " FMT_P " range " FMT_P "-" FMT_P,
        ptr,
        mem_start,
        mem_end_max
    );
    free_blk_header_t *header = (free_blk_header_t *)((char *)ptr - sizeof(size_t));
    BADGEROS_MALLOC_ASSERT_DEBUG(header->size & 1, "realloc: attempting to resize freed pointer " FMT_P, ptr);

    SPIN_LOCK_LOCK(lock);
    char *new_ptr = _malloc(size);
    if (!new_ptr) {
        BADGEROS_MALLOC_MSG_DEBUG("realloc: failed to allocate memory, returning NULL");
        SPIN_LOCK_UNLOCK(lock);
        return NULL;
    }

    size_t old_size = header->size & ~1L;
    BADGEROS_MALLOC_ASSERT_DEBUG(
        (old_size > 0 && old_size < (size_t)mem_end_max - (size_t)mem_start),
        "realloc: block corruption"
    );

    size_t copy_size = old_size < size ? old_size : size;
    __builtin_memcpy(new_ptr, ptr, copy_size);

#if BADGEROS_MALLOC_DEBUG_LEVEL >= BADGEROS_MALLOC_DEBUG_DEBUG
    for (size_t i = 0; i < copy_size; ++i) {
        BADGEROS_MALLOC_ASSERT_DEBUG(
            ((unsigned char *)ptr)[i] == ((unsigned char *)new_ptr)[i],
            "realloc(" FMT_P ", " FMT_ZI "), oldsize " FMT_ZI ", copy size " FMT_ZI ", new pointer: " FMT_P
            " byte " FMT_ZI " memory copy corruption, or data "
            "race on allocation from different thread",
            ptr,
            size,
            old_size,
            copy_size,
            new_ptr,
            i
        );
    }
#endif

    _free(ptr);
    SPIN_LOCK_UNLOCK(lock);
    return new_ptr;
}

void *__wrap_reallocarray(void *ptr, size_t nmemb, size_t size) {
    return __wrap_realloc(ptr, nmemb * size);
}
