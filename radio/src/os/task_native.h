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

#pragma once

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#define TASK_DEFINE_STACK(name, size) void* name

typedef struct task_handle_t {
  pthread_t     _thread;
  uint32_t      _stack_size;
  bool          _created;
  volatile bool _notified;  // __atomic builtins
} task_handle_t;

typedef pthread_mutex_t mutex_handle_t;

static inline uint32_t stackSize()
{
  return 500;
}

static inline uint32_t stackAvailable()
{
  return 500;
}
