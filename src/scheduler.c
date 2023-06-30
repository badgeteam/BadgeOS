#include "scheduler.h"

#include "assertions.h"

#include <inttypes.h>

#define THREAD_RUNNING (1 << 0)  // thread is currently in the scheduling queues
#define THREAD_EXITED (1 << 1)   // thread has finished and is waiting for destruction
#define THREAD_DETACHED (1 << 1) // thread is detached and will self-destroy after exit

// Stack alignment is enforced by the RISC-V calling convention
#define STACK_ALIGNMENT (16)

_Static_assert((STACK_ALIGNMENT & (STACK_ALIGNMENT - 1)) == 0, "STACK_ALIGNMENT must be a power of two!");

struct sched_thread_t
{
    // fixed info:
    process_t *process;
    size_t stack_size;

    // dynamic info:
    uint32_t flags;

    // runtime state:
    uintptr_t stack_pointer;
    uintptr_t instruction_pointer;
};

/// Returns non-0 value if `_V` is aligned to `_A`.
#define is_aligned(_V, _A) (((_V) & ~(_A)) == 0)

static uintptr_t compute_stack_bottom(sched_thread_t *thread)
{
    uintptr_t const base = (uintptr_t)thread;
    uintptr_t const stack_top = base + sizeof(sched_thread_t);
    uintptr_t const stack_bottom = base - thread->stack_size;
    return stack_bottom;
}

// Computes the initial stack pointer that will be valid right before the thread starts.
static uintptr_t compute_stack_top(sched_thread_t *thread)
{
    uintptr_t const base = (uintptr_t)thread;
    uintptr_t const aligned_stack_ptr = (base & ~STACK_ALIGNMENT);
    return aligned_stack_ptr;
}

/// Computes the initial memory location returned by the stack allocator.
static void *restore_stack_bottom(sched_thread_t *thread)
{
    return (void *)compute_stack_top(thread);
}

// Asserts that the threads stack did not under- or overflow.
static inline void probe_stack(sched_thread_t *thread)
{
    assert_dev_drop(thread->stack_pointer <= compute_stack_top(thread));   // Underflow check
    assert_dev_drop(thread->stack_pointer > compute_stack_bottom(thread)); // Overflow check
}

// Pushes a u32 to the stack of the given thread.
static void push_value(sched_thread_t *thread, uint32_t value)
{
    assert_dev_drop(is_aligned(thread->stack_pointer, 4));
    assert_dev_drop(thread->stack_pointer > compute_stack_bottom(thread)); // Overflow check
    thread->stack_pointer -= 4;
    uint32_t *const pointer = (uint32_t *)thread->stack_pointer;
    *pointer = value;
}

// Pops a u32 from the stack of the given thread.
static uint32_t pop_value(sched_thread_t *thread)
{
    assert_dev_drop(is_aligned(thread->stack_pointer, 4));
    assert_dev_drop(thread->stack_pointer < compute_stack_top(thread)); // Underflow check

    uint32_t const *const pointer = (uint32_t const *)thread->stack_pointer;
    thread->stack_pointer += 4;
    return *pointer;
}

void sched_init(badge_err_t *ec)
{
    (void)ec;
}

void sched_run(void)
{
}

sched_thread_t *sched_create_thread(badge_err_t *ec, process_t *process, sched_entry_point_t entry_point, void *arg,
                                    size_t stack_size, sched_thread_priority_t priority)
{
    (void)ec;
    (void)process;
    (void)entry_point;
    (void)stack_size;

    return NULL;
}

void sched_destroy_thread(badge_err_t *ec, sched_thread_t *thread)
{
    (void)ec;
    (void)thread;
}

void sched_detach_thread(badge_err_t *ec, sched_thread_t *thread)
{
    (void)ec;
    (void)thread;
}

void sched_suspend_thread(badge_err_t *ec, sched_thread_t *thread)
{
    (void)ec;
    (void)thread;
}

void sched_resume_thread(badge_err_t *ec, sched_thread_t *thread)
{
    (void)ec;
    (void)thread;
}

void sched_yield()
{
    // TODO: Explicitly invoke the timer ISR here to "preempt" the process early on
}

sched_thread_t *sched_get_current_thread(void)
{
    return NULL;
}

process_t *sched_get_associated_process(sched_thread_t const *thread)
{
    if (thread == NULL)
    {
        return thread->process;
    }
    else
    {
        return NULL;
    }
}