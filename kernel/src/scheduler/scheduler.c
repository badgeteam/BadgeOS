
// SPDX-License-Identifier: MIT

#include "scheduler/scheduler.h"

#include "arrays.h"
#include "assertions.h"
#include "badge_strings.h"
#include "config.h"
#include "cpu/isr.h"
#include "housekeeping.h"
#include "interrupt.h"
#include "isr_ctx.h"
#include "malloc.h"
#include "process/sighandler.h"
#include "scheduler/cpu.h"
#include "scheduler/isr.h"
#include "scheduler/types.h"
#include "smp.h"



// Number of CPUs with running schedulers.
static atomic_int        running_sched_count;
// Number of CPUs ready to perform a load balance.
static atomic_int        loadbalance_ready_count;
// CPU-local scheduler structs.
static sched_cpulocal_t *cpu_ctx;
// Threads list mutex.
static mutex_t           threads_mtx = MUTEX_T_INIT_SHARED;
// Number of threads that exist.
static size_t            threads_len;
// Capacity for thread list.
static size_t            threads_cap;
// Array of all threads that exist.
static sched_thread_t  **threads;
// Thread ID counter.
static atomic_int        tid_counter = 1;
// Unused thread pool mutex.
static mutex_t           unused_mtx  = MUTEX_T_INIT;
// Pool of unused thread handles.
static dlist_t           dead_threads;

// Compare the ID of `sched_thread_t *` to an `int`.
static int tid_int_cmp(void const *a, void const *b) {
    sched_thread_t *thread = *(sched_thread_t **)a;
    tid_t           tid    = (tid_t)(ptrdiff_t)b;
    return thread->id - tid;
}

// Find a thread by TID.
static sched_thread_t *find_thread(tid_t tid) {
    array_binsearch_t res = array_binsearch(threads, sizeof(void *), threads_len, (void *)(ptrdiff_t)tid, tid_int_cmp);
    return res.found ? threads[res.index] : NULL;
}



// Set the context switch to a certain thread.
static inline void set_switch(sched_cpulocal_t *info, sched_thread_t *thread) {
    int pflags = thread->process ? atomic_load(&thread->process->flags) : 0;
    int tflags = atomic_load(&thread->flags);

    // Check for pending signals.
    if (!(tflags & (THREAD_PRIVILEGED | THREAD_SIGHANDLER)) && (pflags & PROC_SIGPEND)) {
        // Process has pending signals to handle first.
        sched_raise_from_isr(thread, false, proc_signal_handler);
    }

    // Set context switch target.
    isr_ctx_t *next = (tflags & THREAD_PRIVILEGED) ? &thread->kernel_isr_ctx : &thread->user_isr_ctx;
    next->cpulocal  = isr_ctx_get()->cpulocal;
    isr_ctx_switch_set(next);

    // Set preemption timer.
    timestamp_us_t timeout = SCHED_MIN_US + SCHED_INC_US * thread->priority;
    if (timeout > info->load_measure_time) {
        timeout = info->load_measure_time;
    }
    timestamp_us_t now = time_us();
    info->last_preempt = now;
    time_set_next_task_switch(now + timeout);
}

// Try to hand a thread off to another CPU.
static bool thread_handoff(sched_thread_t *thread, int cpu, bool force, int max_load) {
    sched_cpulocal_t *info = cpu_ctx + cpu;
    assert_dev_keep(mutex_acquire_shared_from_isr(NULL, &info->run_mtx, TIMESTAMP_US_MAX));

    int  flags      = atomic_load(&info->flags);
    bool is_running = (flags & SCHED_RUNNING) && !(flags & SCHED_EXITING);
    if (!force && !is_running) {
        return false;
    }
    int  usage     = atomic_load(&thread->timeusage.cpu_usage);
    bool has_space = true;

    if (force) {
        // Force handoff; always add to load estimate.
        atomic_fetch_add_explicit(&info->load_estimate, usage, memory_order_relaxed);
    } else {
        // Normal handoff; try to claim space on this CPU for this thread.
        int cur = atomic_load_explicit(&info->load_estimate, memory_order_relaxed);
        int next;
        do {
            next = cur + usage;
            if (next > max_load) {
                has_space = false;
                break;
            }
        } while (!atomic_compare_exchange_strong_explicit(
            &info->load_estimate,
            &cur,
            next,
            memory_order_relaxed,
            memory_order_relaxed
        ));
    }

    if (force || has_space) {
        // Scheduler is running and has capacity for this thread.
        assert_dev_keep(mutex_acquire_from_isr(NULL, &info->incoming_mtx, TIMESTAMP_US_MAX));
        dlist_append(&info->incoming, &thread->node);
        assert_dev_keep(mutex_release_from_isr(NULL, &info->incoming_mtx));
    }

    assert_dev_keep(mutex_release_shared_from_isr(NULL, &info->run_mtx));
    return (flags & SCHED_RUNNING) && !(flags & SCHED_EXITING);
}

// Handle non-normal scheduler flags.
static void sw_handle_sched_flags(timestamp_us_t now, int cur_cpu, sched_cpulocal_t *info, int sched_fl) {
    (void)now;

    if (!(sched_fl & (SCHED_RUNNING | SCHED_STARTING))) {
        // Mark as starting in the first cycle.
        atomic_fetch_or(&info->flags, SCHED_STARTING);

    } else if (sched_fl & SCHED_STARTING) {
        // Mark as running afterwards so CPU0 can free the stack.
        atomic_fetch_xor_explicit(&info->flags, SCHED_RUNNING | SCHED_STARTING, memory_order_relaxed);
        atomic_fetch_add_explicit(&running_sched_count, 1, memory_order_relaxed);

    } else if (sched_fl & SCHED_EXITING) {
        // Exit the scheduler on this CPU.
        assert_dev_keep(mutex_acquire_from_isr(NULL, &info->run_mtx, TIMESTAMP_US_MAX));
        atomic_store_explicit(&info->load_average, 0, memory_order_relaxed);
        atomic_store_explicit(&info->load_estimate, 0, memory_order_relaxed);
        atomic_fetch_sub_explicit(&running_sched_count, 1, memory_order_relaxed);
        atomic_fetch_and(&info->flags, ~(SCHED_RUNNING | SCHED_EXITING));

        // Hand all threads over to other CPUs.
        int cpu = 0;
        dlist_concat(&info->queue, &info->incoming);
        while (info->queue.len) {
            sched_thread_t *thread = (void *)dlist_pop_front(&info->queue);
            do {
                cpu = (cpu + 1) % smp_count;
            } while (cpu == cur_cpu || !thread_handoff(thread, cpu, false, __INT_MAX__));
        }
        assert_dev_keep(mutex_release_from_isr(NULL, &info->run_mtx));

        // Power off this CPU.
        assert_dev_keep(smp_poweroff());
    }
}

// Measure load on this CPU.
static void sw_measure_load(timestamp_us_t now, int cur_cpu, sched_cpulocal_t *info) {
    (void)now;
    (void)cur_cpu;

    // Measure time usage.
    timestamp_us_t  used_time = 0;
    sched_thread_t *thread    = (sched_thread_t *)info->queue.head;
    while (thread) {
        used_time += thread->timeusage.cycle_time;
        thread     = (sched_thread_t *)thread->node.next;
    }

    timestamp_us_t idle_time               = info->idle_thread.timeusage.cycle_time;
    info->idle_thread.timeusage.cycle_time = 0;
    timestamp_us_t total_time              = used_time + idle_time;

    // Account per-thread CPU usage.
    int total_load = 0;
    thread         = (sched_thread_t *)info->queue.head;
    while (thread) {
        timestamp_us_t cpu_time       = thread->timeusage.cycle_time;
        thread->timeusage.cycle_time  = 0;
        int cpu_permil                = cpu_time * 10000 / total_time;
        total_load                   += cpu_permil;
        atomic_store(&thread->timeusage.cpu_usage, cpu_permil);
        thread = (sched_thread_t *)thread->node.next;
    }

    info->load_average  = total_load;
    info->load_estimate = total_load;
}

// Perform load balancing.
static void sw_handle_loadbalance(timestamp_us_t now, int cur_cpu, sched_cpulocal_t *info) {
    (void)now;

    // Wait for all CPUs to have measured their respective load.
    atomic_fetch_add(&loadbalance_ready_count, 1);
    int min = atomic_load(&running_sched_count);
    while (atomic_load(&loadbalance_ready_count) < min) {
        isr_pause();
    }

    // Measure global load average.
    int global_load_average = 0;
    for (int i = 0; i < smp_count; i++) {
        global_load_average += cpu_ctx[i].load_average;
    }
    global_load_average /= atomic_load(&running_sched_count);

    // If this CPU started with less than the load average, don't hand off any threads.
    if (info->load_average <= global_load_average) {
        atomic_fetch_sub(&loadbalance_ready_count, 1);
        return;
    }

    // Hand off threads until either all CPUs expect to meet the load average, or this one dips below.
    sched_thread_t *thread;
    for (size_t i = 0; i < info->queue.len; i++) {
        thread          = (sched_thread_t *)dlist_pop_front(&info->queue);
        bool handoff_ok = false;
        for (int cpu = 0; cpu < smp_count; cpu++) {
            if (cpu == cur_cpu)
                continue;
            if (thread_handoff(thread, cpu, false, global_load_average)) {
                handoff_ok = true;
                break;
            }
        }
        if (!handoff_ok) {
            dlist_append(&info->queue, &thread->node);
        }
    }
    atomic_fetch_sub(&loadbalance_ready_count, 1);
}

// Requests the scheduler to prepare a switch from inside an interrupt routine.
void sched_request_switch_from_isr() {
    timestamp_us_t    now     = time_us();
    int               cur_cpu = smp_cur_cpu();
    sched_cpulocal_t *info    = cpu_ctx + cur_cpu;

    // Check the exiting flag.
    int sched_fl = atomic_load(&info->flags);
    if (sched_fl != SCHED_RUNNING) {
        sw_handle_sched_flags(now, cur_cpu, info, sched_fl);
    }

    // Account thread time usage.
    sched_thread_t *cur_thread = sched_current_thread_unsafe();
    if (cur_thread) {
        timestamp_us_t used               = now - info->last_preempt;
        cur_thread->timeusage.cycle_time += used;
        if (cur_thread->flags & THREAD_PRIVILEGED) {
            cur_thread->timeusage.kernel_time += used;
        } else {
            cur_thread->timeusage.user_time += used;
        }
    }

    // Check for load measurement tiemr.
    if (now >= info->load_measure_time) {
        // Measure load on this CPU.
        sw_measure_load(now, cur_cpu, info);
        // Balance load with other running CPUs.
        sw_handle_loadbalance(now, cur_cpu, info);

        // Set next timestamp to measure load average.
        info->load_measure_time = now + SCHED_LOAD_INTERVAL - (now % SCHED_LOAD_INTERVAL);
    }

    // Check for incoming threads.
    assert_dev_keep(mutex_acquire_from_isr(NULL, &info->incoming_mtx, TIMESTAMP_US_MAX));
    while (info->incoming.len) {
        sched_thread_t *thread = (void *)dlist_pop_front(&info->incoming);
        assert_dev_drop(atomic_load(&thread->flags) & THREAD_RUNNING);
        if (atomic_load(&thread->flags) & THREAD_STARTNOW) {
            dlist_prepend(&info->queue, &thread->node);
        } else {
            dlist_append(&info->queue, &thread->node);
        }
    }
    assert_dev_keep(mutex_release_from_isr(NULL, &info->incoming_mtx));

    // Check for runnable threads.
    while (info->queue.len) {
        // Take the first thread.
        sched_thread_t *thread = (void *)dlist_pop_front(&info->queue);
        int             flags  = atomic_load(&thread->flags);

        // Check for thread exit conditions.
        bool kill_thread = flags & THREAD_EXITING;
        if (thread->process && (atomic_load(&thread->process->flags) & PROC_EXITING)) {
            kill_thread |= !(flags & THREAD_PRIVILEGED);
        }

        if (kill_thread) {
            // Exiting thread/process; clean up thread.
            assert_dev_keep(mutex_acquire_from_isr(NULL, &unused_mtx, TIMESTAMP_US_MAX));
            atomic_fetch_or(&thread->flags, THREAD_EXITED);
            atomic_fetch_and(&thread->flags, ~(THREAD_RUNNING | THREAD_EXITING));
            dlist_append(&dead_threads, &thread->node);
            assert_dev_keep(mutex_release_from_isr(NULL, &unused_mtx));

        } else if (!(flags & THREAD_PRIVILEGED) && (flags & THREAD_SUSPENDING)) {
            // Userspace thread being suspended.
            atomic_fetch_and(&thread->flags, ~(THREAD_RUNNING | THREAD_SUSPENDING));

        } else {
            // Runnable thread found; perform context switch.
            assert_dev_drop(flags & THREAD_RUNNING);
            dlist_append(&info->queue, &thread->node);
            set_switch(info, thread);
            return;
        }
    }

    // If nothing is running on this CPU, run the idle thread.
    set_switch(info, &info->idle_thread);
}

// Scheduler housekeeping.
static void sched_housekeeping(int taskno, void *arg) {
    (void)taskno;
    (void)arg;
    assert_dev_keep(mutex_acquire(NULL, &threads_mtx, TIMESTAMP_US_MAX));

    // Get list of dead threads.
    irq_disable();
    assert_dev_keep(mutex_acquire_from_isr(NULL, &unused_mtx, TIMESTAMP_US_MAX));
    dlist_t         tmp  = DLIST_EMPTY;
    sched_thread_t *node = (void *)dead_threads.head;
    while (node) {
        void *next = (void *)node->node.next;
        if (atomic_load(&node->flags) & THREAD_DETACHED) {
            dlist_remove(&dead_threads, &node->node);
            dlist_append(&tmp, &node->node);
        }
        node = next;
    }
    assert_dev_keep(mutex_release_from_isr(NULL, &unused_mtx));
    irq_enable();

    // Clean up all dead threads.
    while (tmp.len) {
        sched_thread_t *thread = (void *)dlist_pop_front(&tmp);
        free((void *)thread->kernel_stack_bottom);
        if (thread->name) {
            free(thread->name);
        }
        array_binsearch_t res =
            array_binsearch(threads, sizeof(void *), threads_len, (void *)(ptrdiff_t)thread->id, tid_int_cmp);
        assert_dev_drop(res.found);
        array_lencap_remove(&threads, sizeof(void *), &threads_len, &threads_cap, NULL, res.index);
    }

    assert_dev_keep(mutex_release(NULL, &threads_mtx));
}



// Idle function ran when a CPU has no threads.
static void idle_func(void *arg) {
    (void)arg;
    while (1) {
        isr_pause();
        sched_yield();
    }
}

// Global scheduler initialization.
void sched_init() {
    cpu_ctx = malloc(smp_count * sizeof(sched_cpulocal_t));
    assert_always(cpu_ctx);
    mem_set(cpu_ctx, 0, smp_count * sizeof(sched_cpulocal_t));
    for (int i = 0; i < smp_count; i++) {
        cpu_ctx[i].run_mtx      = MUTEX_T_INIT_SHARED;
        cpu_ctx[i].incoming_mtx = MUTEX_T_INIT;
        void *stack             = malloc(8192);
        assert_always(stack);
        cpu_ctx[i].idle_thread.kernel_stack_bottom  = (size_t)stack;
        cpu_ctx[i].idle_thread.kernel_stack_top     = (size_t)stack + 8192;
        cpu_ctx[i].idle_thread.kernel_isr_ctx.flags = ISR_CTX_FLAG_KERNEL;
        cpu_ctx[i].idle_thread.flags                = THREAD_PRIVILEGED;
        sched_prepare_kernel_entry(&cpu_ctx[i].idle_thread, idle_func, NULL);
    }
    hk_add_repeated(0, 1000000, sched_housekeeping, NULL);
}

// Power on and start scheduler on secondary CPUs.
void sched_start_altcpus() {
    int cpu = smp_cur_cpu();
    for (int i = 0; i < smp_count; i++) {
        if (i != cpu) {
            sched_start_on(i);
        }
    }
}

// Power on and start scheduler on another CPU.
bool sched_start_on(int cpu) {
    static mutex_t start_mutex = MUTEX_T_INIT;
    mutex_acquire(NULL, &start_mutex, TIMESTAMP_US_MAX);

    // Tell SMP to power on the other CPU.
    void *tmp_stack  = malloc(CONFIG_STACK_SIZE);
    bool  poweron_ok = smp_poweron(cpu, sched_exec, tmp_stack);
    if (poweron_ok) {
        while (!(atomic_load(&cpu_ctx[cpu].flags) & SCHED_RUNNING)) continue;
    }

    free(tmp_stack);
    mutex_release(NULL, &start_mutex);

    return poweron_ok;
}

// Start executing the scheduler on this CPU.
void sched_exec() {
    timestamp_us_t now = time_us();

    // Allocate CPU-local scheduler data.
    sched_cpulocal_t *info         = cpu_ctx + smp_cur_cpu();
    isr_ctx_get()->cpulocal->sched = info;
    logkf_from_isr(LOG_DEBUG, "Starting scheduler on CPU%{d}", smp_cur_cpu());

    // Set next timestamp to measure load average.
    info->load_average      = 0;
    info->load_estimate     = 0;
    info->load_measure_time = now + SCHED_LOAD_INTERVAL - (now % SCHED_LOAD_INTERVAL);
    atomic_store_explicit(&info->flags, 0, memory_order_release);

    // Start handed over threads or idle until one is handed over to this CPU.
    isr_ctx_get()->flags |= ISR_CTX_FLAG_USE_SP;
    sched_request_switch_from_isr();
    isr_context_switch();
    __builtin_unreachable();
}

// Exit the scheduler and subsequenty shut down the CPU.
void sched_exit(int cpu) {
    assert_dev_keep(mutex_acquire(NULL, &cpu_ctx[cpu].run_mtx, TIMESTAMP_US_MAX));
    atomic_fetch_or_explicit(&cpu_ctx[cpu].flags, SCHED_EXITING, memory_order_relaxed);
    assert_dev_keep(mutex_release(NULL, &cpu_ctx[cpu].run_mtx));
}


// Create a new suspended userland thread.
tid_t thread_new_user(
    badge_err_t *ec, char const *name, process_t *process, size_t user_entrypoint, size_t user_arg, int priority
) {
    // Allocate thread.
    sched_thread_t *thread = malloc(sizeof(sched_thread_t));
    if (!thread) {
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOMEM);
        return 0;
    }
    mem_set(thread, 0, sizeof(sched_thread_t));

    thread->kernel_stack_bottom = (size_t)malloc(CONFIG_STACK_SIZE);
    if (!thread->kernel_stack_bottom) {
        free(thread);
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOMEM);
        return 0;
    }

    if (name) {
        size_t name_len = cstr_length(name);
        thread->name    = malloc(name_len + 1);
        if (!thread->name) {
            free((void *)thread->kernel_stack_bottom);
            free(thread);
            badge_err_set(ec, ELOC_THREADS, ECAUSE_NOMEM);
            return 0;
        }
        cstr_copy(thread->name, name_len + 1, name);
    }

    thread->priority              = priority;
    thread->process               = process;
    thread->id                    = atomic_fetch_add(&tid_counter, 1);
    thread->kernel_stack_top      = thread->kernel_stack_bottom + CONFIG_STACK_SIZE;
    thread->kernel_isr_ctx.flags  = ISR_CTX_FLAG_KERNEL;
    thread->kernel_isr_ctx.thread = thread;
    thread->user_isr_ctx.thread   = thread;
    thread->user_isr_ctx.mpu_ctx  = &process->memmap.mpu_ctx;
    sched_prepare_user_entry(thread, user_entrypoint, user_arg);

    assert_dev_keep(mutex_acquire(NULL, &threads_mtx, TIMESTAMP_US_MAX));
    bool success = array_lencap_insert(&threads, sizeof(void *), &threads_len, &threads_cap, &thread, threads_len);
    assert_dev_keep(mutex_release(NULL, &threads_mtx));
    if (!success) {
        if (thread->name) {
            free(thread->name);
        }
        free((void *)thread->kernel_stack_bottom);
        free(thread);
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOMEM);
        return 0;
    }

    return thread->id;
}

// Create new suspended kernel thread.
tid_t thread_new_kernel(badge_err_t *ec, char const *name, sched_entry_t entrypoint, void *arg, int priority) {
    // Allocate thread.
    sched_thread_t *thread = malloc(sizeof(sched_thread_t));
    if (!thread) {
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOMEM);
        return 0;
    }
    mem_set(thread, 0, sizeof(sched_thread_t));

    thread->kernel_stack_bottom = (size_t)malloc(CONFIG_STACK_SIZE);
    if (!thread->kernel_stack_bottom) {
        free(thread);
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOMEM);
        return 0;
    }

    if (name) {
        size_t name_len = cstr_length(name);
        thread->name    = malloc(name_len + 1);
        if (!thread->name) {
            free((void *)thread->kernel_stack_bottom);
            free(thread);
            badge_err_set(ec, ELOC_THREADS, ECAUSE_NOMEM);
            return 0;
        }
        cstr_copy(thread->name, name_len + 1, name);
    }

    thread->priority               = priority;
    thread->id                     = atomic_fetch_add(&tid_counter, 1);
    thread->kernel_stack_top       = thread->kernel_stack_bottom + CONFIG_STACK_SIZE;
    thread->kernel_isr_ctx.flags   = ISR_CTX_FLAG_KERNEL;
    thread->kernel_isr_ctx.thread  = thread;
    thread->flags                 |= THREAD_PRIVILEGED | THREAD_KERNEL;
    sched_prepare_kernel_entry(thread, entrypoint, arg);

    assert_dev_keep(mutex_acquire(NULL, &threads_mtx, TIMESTAMP_US_MAX));
    bool success = array_lencap_insert(&threads, sizeof(void *), &threads_len, &threads_cap, &thread, threads_len);
    assert_dev_keep(mutex_release(NULL, &threads_mtx));
    if (!success) {
        if (thread->name) {
            free(thread->name);
        }
        free((void *)thread->kernel_stack_bottom);
        free(thread);
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOMEM);
        return 0;
    }

    logkf(LOG_DEBUG, "Kernel thread #%{d} '%{cs}' @0x%{size;x} created", thread->id, thread->name, thread);

    badge_err_set_ok(ec);
    return thread->id;
}

// Do not wait for thread to be joined; clean up immediately.
void thread_detach(badge_err_t *ec, tid_t tid) {
    assert_always(mutex_acquire_shared(NULL, &threads_mtx, TIMESTAMP_US_MAX));
    sched_thread_t *thread = find_thread(tid);
    if (thread) {
        atomic_fetch_or(&thread->flags, THREAD_DETACHED);
        badge_err_set_ok(ec);
    } else {
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOTFOUND);
    }
    assert_always(mutex_release_shared(NULL, &threads_mtx));
}


// Pauses execution of a user thread.
void thread_suspend(badge_err_t *ec, tid_t tid) {
    assert_always(mutex_acquire_shared(NULL, &threads_mtx, TIMESTAMP_US_MAX));
    sched_thread_t *thread = find_thread(tid);
    if (thread) {
        if (thread->flags & THREAD_KERNEL) {
            badge_err_set(ec, ELOC_THREADS, ECAUSE_ILLEGAL);
        } else {
            int exp;
            do {
                exp = atomic_load(&thread->flags);
            } while (!atomic_compare_exchange_strong(&thread->flags, &exp, exp | THREAD_SUSPENDING));
            badge_err_set_ok(ec);
        }
    } else {
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOTFOUND);
    }
    assert_always(mutex_release_shared(NULL, &threads_mtx));
}

// Try to mark a thread as running if a thread is allowed to be resumed.
static bool thread_try_mark_running(sched_thread_t *thread, bool now) {
    int cur = atomic_load(&thread->flags);
    int nextval;
    do {
        if (cur & (THREAD_EXITED | THREAD_EXITING | THREAD_RUNNING)) {
            return false;
        }
        nextval = (cur | THREAD_RUNNING) & ~THREAD_SUSPENDING;
        if (now) {
            nextval |= THREAD_STARTNOW;
        }
    } while (!atomic_compare_exchange_strong(&thread->flags, &cur, nextval));
    return true;
}

// Resumes a previously suspended thread or starts it.
static void thread_resume_impl(badge_err_t *ec, tid_t tid, bool now) {
    assert_always(mutex_acquire_shared(NULL, &threads_mtx, TIMESTAMP_US_MAX));
    sched_thread_t *thread = find_thread(tid);
    if (thread) {
        if (thread_try_mark_running(thread, now)) {
            irq_disable();
            thread_handoff(thread, smp_cur_cpu(), true, __INT_MAX__);
            irq_enable();
        }
        badge_err_set_ok(ec);
    } else {
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOTFOUND);
    }
    assert_always(mutex_release_shared(NULL, &threads_mtx));
}

// Resumes a previously suspended thread or starts it.
void thread_resume(badge_err_t *ec, tid_t tid) {
    thread_resume_impl(ec, tid, false);
}

// Resumes a previously suspended thread or starts it.
// Immediately schedules the thread instead of putting it in the queue first.
void thread_resume_now(badge_err_t *ec, tid_t tid) {
    thread_resume_impl(ec, tid, true);
}

// Returns whether a thread is running; it is neither suspended nor has it exited.
bool thread_is_running(badge_err_t *ec, tid_t tid) {
    assert_always(mutex_acquire_shared(NULL, &threads_mtx, TIMESTAMP_US_MAX));
    sched_thread_t *thread = find_thread(tid);
    bool            res    = false;
    if (thread) {
        res = !!(atomic_load(&thread->flags) & THREAD_RUNNING);
        badge_err_set_ok(ec);
    } else {
        badge_err_set(ec, ELOC_THREADS, ECAUSE_NOTFOUND);
    }
    assert_always(mutex_release_shared(NULL, &threads_mtx));
    return res;
}


// Returns the current thread ID.
tid_t sched_current_tid() {
    return isr_ctx_get()->thread->id;
}

// Returns the current thread struct.
sched_thread_t *sched_current_thread() {
    irq_disable();
    sched_thread_t *thread = isr_ctx_get()->thread;
    irq_enable();
    return thread;
}

// Returns the current thread without using a critical section.
sched_thread_t *sched_current_thread_unsafe() {
    return isr_ctx_get()->thread;
}

// Returns the associated thread struct.
sched_thread_t *sched_get_thread(tid_t tid) {
    assert_always(mutex_acquire_shared(NULL, &threads_mtx, TIMESTAMP_US_MAX));
    sched_thread_t *thread = find_thread(tid);
    assert_always(mutex_release_shared(NULL, &threads_mtx));
    return thread;
}


// Explicitly yield to the scheduler; the scheduler may run other threads without waiting for preemption.
// Use this function to reduce the CPU time used by a thread.
void sched_yield() {
    irq_disable();
    sched_request_switch_from_isr();
    isr_context_switch();
}

// Exits the current thread.
// If the thread is detached, resources will be cleaned up.
void thread_exit(int code) {
    irq_disable();
    sched_thread_t *thread = isr_ctx_get()->thread;
    thread->exit_code      = code;
    atomic_fetch_or(&thread->flags, THREAD_EXITING);
    sched_request_switch_from_isr();
    isr_context_switch();
    __builtin_unreachable();
}

// Wait for another thread to exit.
void thread_join(tid_t tid) {
    while (1) {
        assert_always(mutex_acquire_shared(NULL, &threads_mtx, TIMESTAMP_US_MAX));
        sched_thread_t *thread = find_thread(tid);
        if (thread) {
            if (atomic_load(&thread->flags) & THREAD_EXITED) {
                atomic_fetch_or(&thread->flags, THREAD_DETACHED);
                assert_always(mutex_release_shared(NULL, &threads_mtx));
                return;
            }
        } else {
            assert_always(mutex_release_shared(NULL, &threads_mtx));
            return;
        }
        assert_always(mutex_release_shared(NULL, &threads_mtx));
        sched_yield();
    }
}
