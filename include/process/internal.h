
// SPDX-License-Identifier: MIT

#pragma once

#include "process/process.h"

// Suspend all threads for a process except the current.
void proc_suspend(process_t *process, sched_thread_t *current);
// Resume all threads for a process.
void proc_resume(process_t *process);

// Release all process runtime resources (threads, memory, files, etc.).
// Does not remove args, exit code, etc.
void proc_delete_runtime(process_t *process);
