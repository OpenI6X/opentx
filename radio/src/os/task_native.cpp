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

extern uint8_t simuSleep(uint32_t ms);

static thread_local task_handle_t* _current = nullptr;

struct run_context {
  task_func_t    func;
  task_handle_t* handle;
};

static void* _task_stub(void* p)
{
  run_context ctx = *(run_context*)p;
  delete (run_context*)p;
  _current = ctx.handle;
  ctx.func();
  return nullptr;
}

void task_create(task_handle_t* h, task_func_t func, const char* name,
                 void* stack, unsigned stack_size)
{
  (void)stack;
  h->_stack_size = stack_size;
  h->_notified = false;
  h->_created = true;
  pthread_create(&h->_thread, nullptr, _task_stub, new run_context{func, h});
#ifdef __linux__
  pthread_setname_np(h->_thread, name);
#else
  (void)name;
#endif
}

unsigned task_get_stack_usage(task_handle_t* h)
{
  return h->_stack_size / 2;
}

unsigned task_get_stack_size(task_handle_t* h)
{
  return h->_stack_size;
}

bool scheduler_is_running()
{
  return true;
}

void mutex_create(mutex_handle_t* h)
{
  pthread_mutex_init(h, nullptr);
}

bool mutex_lock(mutex_handle_t* h)
{
  return pthread_mutex_lock(h) == 0;
}

void mutex_unlock(mutex_handle_t* h)
{
  pthread_mutex_unlock(h);
}

void task_notify(task_handle_t* h)
{
  if (h->_created) {
    __atomic_store_n(&h->_notified, true, __ATOMIC_SEQ_CST);
  }
}

void task_notify_isr(task_handle_t* h)
{
  task_notify(h);
}

// polls each 1 ms (as the SIMU timer task does)
bool task_wait_notify(uint32_t timeout_ms)
{
  for (uint32_t t = 0;; t++) {
    if (_current && __atomic_exchange_n(&_current->_notified, false, __ATOMIC_SEQ_CST)) {
      return true;
    }
    if (t >= timeout_ms) {
      return false;
    }
    simuSleep(1);
  }
}
