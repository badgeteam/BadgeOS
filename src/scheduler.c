#include "scheduler.h"

#include "assertions.h"
#include "attributes.h"
#include "kernel_ctx.h"
#include "list.h"
#include "meta.h"

#include <inttypes.h>

/// Returns non-0 value if `V` is aligned to `A`.
#define is_aligned(V, A) (((V) & ((A)-1)) == 0)

enum
{
    // thread is currently in the scheduling queues
    THREAD_RUNNING = (1 << 0),

    // thread has finished and is waiting for destruction
    THREAD_COMPLETED = (1 << 1),

    // thread is detached and will self-destroy after exit
    THREAD_DETACHED = (1 << 2)
};

// Stack alignment is enforced by the RISC-V calling convention
enum
{
    STACK_ALIGNMENT = 16,
};

static_assert((STACK_ALIGNMENT & (STACK_ALIGNMENT - 1)) == 0, "STACK_ALIGNMENT must be a power of two!");

enum
{
    SCHED_THREAD_NAME_LEN = 32,
};

struct sched_thread_t
{
    // fixed info:
    process_t *process;
    uintptr_t stack_bottom;
    uintptr_t stack_top;
    sched_thread_priority_t priority;

    // dynamic info:
    uint32_t flags;
    dlist_node_t schedule_node;
    uint32_t exit_code;

    // runtime state:
    kernel_ctx_t kernel_ctx;

#ifndef NDEBUG
    // debug info:
    char name[SCHED_THREAD_NAME_LEN];
#endif
};

// List of currently queued threads. `head` will be queued next, `tail` will be
// queued last.
static dlist_t thread_wait_queue = DLIST_EMPTY;

enum
{
    // Size of the
    idle_task_stack_len = 128
};
static uint8_t idle_task_stack[idle_task_stack_len] ALIGNED_TO(STACK_ALIGNMENT);

static_assert(is_aligned(idle_task_stack_len, STACK_ALIGNMENT));

// The scheduler must schedule something, and the idle task is what
// the scheduler will schedule when nothing can be scheduled.
static sched_thread_t idle_task = {
    .name = "idle",
    .stack_bottom = (uintptr_t)&idle_task_stack,
    .stack_top = (uintptr_t)&idle_task_stack + idle_task_stack_len,
};

// Enters a scheduler-local critical section that cannot be interrupted from the
// scheduler itself. Call `leave_critical_section` after the critical section
// has ended.
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

static sched_thread_t *thread_alloc(void)
{
    // TODO: Allocate thread here
    return NULL;
}

static void thread_free(sched_thread_t *thread)
{
    // TODO: Free thread here
}

static void idle_thread_function(void *arg)
{
    (void)arg;
    while (true)
    {
        asm volatile("" ::: "memory"); // make the loop not be undefined behaviour
    }
}

// The trampoline is used to jump into the thread code and return from it,
// ensuring that we can detect when a thread has exited.
static void thread_trampoline(sched_entry_point_t ep, void *arg)
{
    assert_always(ep != NULL);

    sched_thread_t *const this_thread = sched_get_current_thread();
    assert_always(this_thread != NULL);

    ep(arg);

    // make sure the thread is always exited properly
    sched_exit(0);
}

static void setup_thread_state(kernel_ctx_t *const ctx, uintptr_t initial_stack_pointer,
                               sched_entry_point_t entry_point, void *arg)
{
    // TODO: Implement that function here cross-platform for other CPUs or move it to the cpu-port

    ctx->regs->pc = (uintptr_t)thread_trampoline;
    ctx->regs->sp = initial_stack_pointer;
    ctx->regs->a0 = (uintptr_t)entry_point;
    ctx->regs->a1 = (uintptr_t)arg;

    // TODO: Also set up GP and TP for RISC-V
}

static void trigger_task_switch_isr(void)
{
    // TODO: Explicitly invoke the timer ISR here to "preempt" the process early
    // on
}

static void destroy_thread(sched_thread_t *thread)
{
    assert_dev_drop(thread != NULL);

    if ((thread->flags & THREAD_RUNNING) == 0)
    {
        // thread is still running, we have to remove it from the thread queue:
        sched_suspend_thread(NULL, thread);
    }

    // At last, we free the memory:
    thread_free(thread);
}

sched_thread_t *sched_get_current_thread(void)
{
    enter_critical_section();
    kernel_ctx_t *const kernel_ctx = kernel_ctx_get();
    leave_critical_section();
    return field_parent_ptr(sched_thread_t, kernel_ctx, kernel_ctx);
}

void sched_init(badge_err_t *const ec)
{
    setup_thread_state(&idle_task.kernel_ctx, idle_task.stack_top, idle_thread_function, NULL);

    badge_err_set_ok(ec);
}

void sched_exec(void)
{
    trigger_task_switch_isr();

    // we can never reach this line, as the ISR will switch into the idle task
    __builtin_unreachable();
}

void sched_request_switch_from_isr(void)
{
    sched_thread_t *const current_thread = sched_get_current_thread();
    if (current_thread != NULL)
    {
        if ((current_thread->flags & THREAD_RUNNING) != 0)
        {

            // if we have a current thread, append it to the wait queue again before
            // popping the next task. This is necessary as we if we only have a single
            // task, that should be scheduled again. Otheriwse, `dlist_pop_front` would
            // return `NULL` instead of `current_thread`.
            dlist_append(&thread_wait_queue, &current_thread->schedule_node);
        }
        else
        {
            // current thread is dead, we don't push it into the scheduler again

            if ((current_thread->flags & THREAD_DETACHED))
            {
                destroy_thread(current_thread);
            }
        }
    }

    dlist_node_t *const next_thread_node = dlist_pop_front(&thread_wait_queue);
    if (next_thread_node != NULL)
    {
        sched_thread_t *const next_thread = field_parent_ptr(sched_thread_t, schedule_node, next_thread_node);

        // Set the switch target
        kernel_ctx_switch_set(&next_thread->kernel_ctx);

        // TODO: Set timer timeout here!
    }
    else
    {
        // nothing to do, switch to idle task:

        kernel_ctx_switch_set(&idle_task.kernel_ctx);
        // TODO: Set timer timeout here!
    }
}

sched_thread_t *sched_create_userland_thread(badge_err_t *const ec, process_t *const process,
                                             const sched_entry_point_t entry_point, void *const arg,
                                             const sched_thread_priority_t priority)
{
    (void)process;
    (void)entry_point;
    (void)arg;
    (void)priority;

    badge_err_set(ec, ELOC_THREADS, ECAUSE_UNSUPPORTED);

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

    badge_err_set(ec, ELOC_THREADS, ECAUSE_UNSUPPORTED);

    return NULL;
}

void sched_destroy_thread(badge_err_t *ec, sched_thread_t *thread)
{
    assert_dev_drop(thread != NULL);

    (void)thread;

    if (thread == sched_get_current_thread())
    {
        sched_detach_thread(ec, thread);
        if (!badge_err_is_ok(ec))
        {
            return;
        }
        sched_exit(0);
    }

    destroy_thread(thread);
    badge_err_set_ok(ec);
}

void sched_detach_thread(badge_err_t *ec, sched_thread_t *thread)
{
    assert_dev_drop(thread != NULL);

    enter_critical_section();
    (void)thread;
    leave_critical_section();

    badge_err_set(ec, ELOC_THREADS, ECAUSE_UNSUPPORTED);
}

void sched_suspend_thread(badge_err_t *const ec, sched_thread_t *const thread)
{
    assert_dev_drop(thread != NULL);

    enter_critical_section();
    (void)thread;
    leave_critical_section();

    badge_err_set(ec, ELOC_THREADS, ECAUSE_UNSUPPORTED);
}

void sched_resume_thread(badge_err_t *const ec, sched_thread_t *const thread)
{
    assert_dev_drop(thread != NULL);
    enter_critical_section();

    if ((thread->flags & THREAD_COMPLETED) != 0)
    {
        badge_err_set(ec, ELOC_THREADS, ECAUSE_ILLEGAL);
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

void sched_yield(void)
{
    sched_thread_t *const current_thread = sched_get_current_thread();
    assert_always(current_thread != NULL);

    trigger_task_switch_isr();
}

void sched_exit(uint32_t exit_code)
{
    sched_thread_t *const current_thread = sched_get_current_thread();
    assert_always(current_thread != NULL);

    current_thread->exit_code = exit_code;
    current_thread->flags |= THREAD_COMPLETED;

    sched_yield();

    // hint the compiler that we cannot reach this part of the code and
    // it will never be reached:
    __builtin_unreachable();
}
