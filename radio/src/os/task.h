/*
 * Copyright (C) OpenI6X / OpenTX
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

// OS abstraction, EdgeTX os/ layout and names (radio/src/os/task.h).
// Differences: no task priority (TinyK: priority = creation order), plus
// per-task notifications and optional CPU time counters.

#pragma once

#include <stdint.h>
#include <stdbool.h>

// OS specific implementation
#if defined(SIMU)
  #include "task_native.h"
#elif defined(RTOS_TINYK)
  #include "task_tinyk.h"
#endif

typedef void (*task_func_t)();

// Priority = creation order: the first task created has the highest priority.
void task_create(task_handle_t* h, task_func_t func, const char* name,
                 void* stack, unsigned stack_size);

// As in EdgeTX: bytes of stack never used so far (high-water mark).
unsigned task_get_stack_usage(task_handle_t* h);
unsigned task_get_stack_size(task_handle_t* h);  // bytes

bool scheduler_is_running();

void mutex_create(mutex_handle_t* h);
bool mutex_lock(mutex_handle_t* h);
void mutex_unlock(mutex_handle_t* h);

// Task notification (binary, like a FreeRTOS direct-to-task notification
// used as a binary semaphore). One pending bit per task: notifying a task
// that is busy keeps it pending, the next task_wait_notify() returns at once.
void task_notify(task_handle_t* h);      // task context
void task_notify_isr(task_handle_t* h);  // ISR context
// Calling task waits up to timeout_ms; true = notified (bit cleared),
// false = timeout.
bool task_wait_notify(uint32_t timeout_ms);

#if defined(TASK_CPU_STATS)
// Free-running CPU time counters (wrap around), TASK_CPU_UNITS_PER_MS units.
// ISR time is charged to the interrupted task or idle.
uint32_t task_get_cpu_time(task_handle_t* h);
uint32_t task_get_idle_time();
#endif
