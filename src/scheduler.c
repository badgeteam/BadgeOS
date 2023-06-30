#include "scheduler.h"

#include "assertions.h"
#include "list.h"
#include "meta.h"
#include "attributes.h"

#include <inttypes.h>

#define THREAD_RUNNING (1 << 0)  // thread is currently in the scheduling queues
#define THREAD_EXITED (1 << 1)   // thread has finished and is waiting for destruction
#define THREAD_DETACHED (1 << 2) // thread is detached and will self-destroy after exit

// Stack alignment is enforced by the RISC-V calling convention
#define STACK_ALIGNMENT (16)

static_assert((STACK_ALIGNMENT & (STACK_ALIGNMENT - 1)) == 0, "STACK_ALIGNMENT must be a power of two!");

struct sched_thread_t
{
    // fixed info:
    process_t *process;
    size_t stack_size;

    // dynamic info:
    uint32_t flags;
    dlist_node_t schedule_node;

    // runtime state:
    uintptr_t stack_pointer;
    uintptr_t instruction_pointer;
};

/// Returns non-0 value if `V` is aligned to `A`.
#define is_aligned(V, A) (((V) & ~(A)) == 0)

static inline uintptr_t FORCEINLINE compute_stack_bottom(sched_thread_t *const thread)
{
    assert_dev_drop(thread != NULL);

    uintptr_t const base = (uintptr_t)thread;
    uintptr_t const stack_top = base + sizeof(sched_thread_t);
    uintptr_t const stack_bottom = base - thread->stack_size;
    return stack_bottom;
}

// Computes the initial stack pointer that will be valid right before the thread starts.
static inline uintptr_t FORCEINLINE compute_stack_top(sched_thread_t *const thread)
{
    assert_dev_drop(thread != NULL);

    uintptr_t const base = (uintptr_t)thread;
    uintptr_t const aligned_stack_ptr = (base & ~STACK_ALIGNMENT);
    return aligned_stack_ptr;
}

/// Computes the initial memory location returned by the stack allocator.
static inline void *FORCEINLINE restore_stack_bottom(sched_thread_t *const thread)
{
    return (void *)compute_stack_top(thread);
}

// Asserts that the threads (stored) stack pointer did not under- or overflow.
static inline void validate_stack_pointer(sched_thread_t *const thread)
{
    assert_dev_drop(thread != NULL);
    assert_dev_drop(thread->stack_pointer <= compute_stack_top(thread));   // Underflow check
    assert_dev_drop(thread->stack_pointer > compute_stack_bottom(thread)); // Overflow check
}

// Pushes a u32 to the stack of the given thread.
static void push_value(sched_thread_t *const thread, uint32_t const value)
{
    assert_dev_drop(thread != NULL);

    assert_dev_drop(is_aligned(thread->stack_pointer, 4));
    assert_dev_drop(thread->stack_pointer > compute_stack_bottom(thread)); // Overflow check
    thread->stack_pointer -= 4;
    uint32_t *const pointer = (uint32_t *)thread->stack_pointer;
    *pointer = value;
}

// Pops a u32 from the stack of the given thread.
static uint32_t pop_value(sched_thread_t *const thread)
{
    assert_dev_drop(thread != NULL);
    assert_dev_drop(is_aligned(thread->stack_pointer, 4));
    assert_dev_drop(thread->stack_pointer < compute_stack_top(thread)); // Underflow check

    uint32_t const *const pointer = (uint32_t const *)thread->stack_pointer;
    thread->stack_pointer += 4;
    return *pointer;
}

// Enters a scheduler-local critical section that cannot be interrupted from the scheduler itself.
// Call `leave_critical_section` after the critical section has ended.
//
// During a critical section, no thread switches can occurr.
static void enter_critical_section(void)
{
    //
}

static void leave_critical_section(void)
{
    //
}

// List of currently queued threads. `head` will be queued next, `tail` will be queued last.
static dlist_t thread_wait_queue = DLIST_EMPTY;

static sched_thread_t *current_thread = NULL;

void sched_init(badge_err_t *const ec)
{
    badge_err_set_ok(ec);
}

void sched_run(void)
{
    if (thread_wait_queue.len == 0)
    {
        return;
    }

    // kick-off the multithreading here

    assert_dev_drop(thread_wait_queue.len == 0);
}

sched_thread_t *sched_create_userland_thread(badge_err_t *const ec, process_t *const process,
                                             sched_entry_point_t const entry_point, void *const arg,
                                             sched_thread_priority_t const priority)
{
    (void)process;
    (void)entry_point;
    (void)arg;
    (void)priority;

    badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_UNSUPPORTED);

    return NULL;
}

sched_thread_t *sched_create_kernel_thread(badge_err_t *const ec, sched_entry_point_t const entry_point,
                                           void *const arg, void *const stack_bottom, size_t const stack_size,
                                           sched_thread_priority_t const priority)
{
    (void)entry_point;
    (void)arg;
    (void)priority;
    (void)stack_bottom;
    (void)stack_size;

    badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_UNSUPPORTED);

    return NULL;
}

void sched_destroy_thread(badge_err_t *ec, sched_thread_t *thread)
{
    assert_dev_drop(thread != NULL);

    (void)thread;

    badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_UNSUPPORTED);
}

void sched_detach_thread(badge_err_t *ec, sched_thread_t *thread)
{
    assert_dev_drop(thread != NULL);

    enter_critical_section();
    (void)thread;
    leave_critical_section();

    badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_UNSUPPORTED);
}

void sched_suspend_thread(badge_err_t *const ec, sched_thread_t *const thread)
{
    assert_dev_drop(thread != NULL);

    enter_critical_section();
    (void)thread;
    leave_critical_section();

    badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_UNSUPPORTED);
}

void sched_resume_thread(badge_err_t *const ec, sched_thread_t *const thread)
{
    assert_dev_drop(thread != NULL);
    enter_critical_section();

    if ((thread->flags & THREAD_EXITED) != 0)
    {
        badge_err_set(ec, 0, 0);
        leave_critical_section();
        return;
    }

    if ((thread->flags & THREAD_RUNNING) == 0)
    {
        // TODO: Implement thread resumption
        thread->flags |= THREAD_RUNNING;
    }

    leave_critical_section();

    badge_err_set(ec, ELOC_UNKNOWN, ECAUSE_UNSUPPORTED);
}

// Probes the current stack and tests if it did over or underflow.
// Must be called from a critical section!
static inline void FORCEINLINE probe_stack(void)
{
    uintptr_t stack_pointer;

    assert_dev_drop(stack_pointer <= compute_stack_top(current_thread));   // Underflow check
    assert_dev_drop(stack_pointer > compute_stack_bottom(current_thread)); // Overflow check
}

void sched_yield(void)
{
    enter_critical_section();
    assert_always(current_thread != NULL);

    probe_stack();

    // TODO: Explicitly invoke the timer ISR here to "preempt" the process early on

    probe_stack();

    leave_critical_section();
}

sched_thread_t *sched_get_current_thread(void)
{
    enter_critical_section();
    sched_thread_t *const current = current_thread;
    leave_critical_section();
    return current;
}

process_t *sched_get_associated_process(sched_thread_t const *const thread)
{
    enter_critical_section();
    process_t *process = NULL;
    if (thread != NULL)
    {
        process = thread->process;
    }
    leave_critical_section();
    return process;
}
