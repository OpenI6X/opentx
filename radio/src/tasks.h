/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
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

#ifndef _TASKS_H_
#define _TASKS_H_

#include "rtos.h"
#include "os/task.h"
#include "os/time.h"
#include "os/sleep.h"
#include "os/timer.h"

// stack sizes should be in multiples of 8 for better alignment
// expressed in integers resulting size is 4 times bigger
#define MENUS_STACK_SIZE       256 // 172 analyzed max
#define MIXER_STACK_SIZE       200 // 132 analyzed max
#define AUDIO_STACK_SIZE       400
#define CLI_STACK_SIZE         1024  // only consumed with CLI build option

// Task priority = creation order in tasksStart() (first = highest).
// The timer task (timerTaskId, TIMER_STACK_SIZE) is in os/timer.

extern task_handle_t menusTaskId;
extern task_handle_t mixerTaskId;
extern task_handle_t audioTaskId;

extern mutex_handle_t mixerMutex;

void tasksStart();

extern volatile uint16_t timeForcePowerOffPressed;
inline void resetForcePowerOffRequest()
{
  timeForcePowerOffPressed = 0;
}

#endif // _TASKS_H_
