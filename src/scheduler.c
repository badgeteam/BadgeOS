#include "scheduler.h"

struct sched_thread_t
{
    process_t *process;
    size_t stack_pointer;
};

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