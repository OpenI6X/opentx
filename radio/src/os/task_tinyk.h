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

#include "tinyk.h"

// 8-byte aligned (AAPCS; printf of %f fails otherwise). tk_create() paints it.
#define TASK_DEFINE_STACK(name, size) uint32_t __attribute__((aligned(8))) name[size]

typedef struct task_handle_t {
  uint32_t* _stack;       // NULL = not created (static zero init)
  uint16_t  _stack_size;  // words
  uint8_t   _id;          // TinyK task index = priority
} task_handle_t;

typedef tk_mutex_t mutex_handle_t;

#if defined(TK_IDLE_STATS)
  #define TASK_CPU_STATS        1
  #define TASK_CPU_UNITS_PER_MS (TK_IDLE_CLOCK_HZ / 1000u)
#endif

// main (MSP) stack: ISRs and code before the scheduler starts
uint32_t stackSize();       // words
uint32_t stackAvailable();  // bytes never used
