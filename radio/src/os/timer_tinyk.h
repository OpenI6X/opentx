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

// timer task (FreeRTOS: configTIMER_TASK_STACK_DEPTH), words.
// 112 analyzed max: telemetry callback AFHDS2A GPS path 336 B (1 recursion)
// + timer task 32 B + 16 B libgcc + 64 B exception/context frames
#define TIMER_STACK_SIZE  128
