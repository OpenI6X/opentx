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

#include "task.h"

void task_create(task_handle_t* h, task_func_t func, const char* name,
                 void* stack, unsigned stack_size)
{
  (void)name;
  uint8_t id = tk_create(func, (uint32_t*)stack, stack_size);
  if (id == 0xFF) {
    return;
  }
  h->_id = id;
  h->_stack_size = stack_size;
  h->_stack = (uint32_t*)stack;
}

unsigned task_get_stack_usage(task_handle_t* h)
{
  return h->_stack ? tk_stack_free(h->_stack, h->_stack_size) : 0;
}

unsigned task_get_stack_size(task_handle_t* h)
{
  return h->_stack_size * sizeof(uint32_t);
}

bool scheduler_is_running()
{
  return tk_started_get();
}

void mutex_create(mutex_handle_t* h)
{
  tk_mutex_init(h);
}

bool mutex_lock(mutex_handle_t* h)
{
  tk_mutex_lock(h);
  return true;
}

void mutex_unlock(mutex_handle_t* h)
{
  tk_mutex_unlock(h);
}

// TinyK: tk_notify() is safe from both contexts
void task_notify(task_handle_t* h)
{
  if (h->_stack) {
    tk_notify(h->_id);
  }
}

void task_notify_isr(task_handle_t* h)
{
  if (h->_stack) {
    tk_notify(h->_id);
  }
}

bool task_wait_notify(uint32_t timeout_ms)
{
  return tk_notify_wait(timeout_ms);  // 1 tick = 1 ms
}

#if defined(TASK_CPU_STATS)
uint32_t task_get_cpu_time(task_handle_t* h)
{
  return h->_stack ? tk_task_time[h->_id] : 0;
}

uint32_t task_get_idle_time()
{
  return tk_idle_time;
}
#endif

extern int _estack;
extern int _main_stack_start;

uint32_t stackSize()
{
  return ((unsigned char *)&_estack - (unsigned char *)&_main_stack_start) / 4;
}

uint32_t stackAvailable()
{
  return tk_stack_free((const uint32_t *)&_main_stack_start, stackSize());
}
