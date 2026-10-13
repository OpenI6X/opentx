/*
 * Software timers - shared by the TinyK, CoOS and SIMU backends
 *
 * Copyright (C) OpenI6X / OpenTX
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * FreeRTOS-like: static timer objects with a callback, one-shot or
 * auto-reload, (re)started / stopped from task or ISR context. Callbacks run
 * in timerTask (created in tasksStart, priority between mixer and menus),
 * never in an ISR.
 *
 * A one-shot timer with period 0 runs its callback as soon as possible after
 * timer_start(): use it to defer work out of an ISR (e.g. telemetry parsing).
 *
 * Callbacks run one after another in one task:
 *   - keep each well below 1 ms (they delay each other and telemetry),
 *   - never block, never take mixerMutex (menus may hold it for a model load).
 * Times are in ms, resolution is one RTOS tick (TinyK 1 ms, CoOS 2 ms).
 * Use through the RTOS_*_TIMER wrappers in rtos.h.
 */

#ifndef _RTOS_TIMER_H_
#define _RTOS_TIMER_H_

#include <stdint.h>

typedef void (*timer_callback_t)();

typedef struct rtos_timer {
  struct rtos_timer * next;      // active list, owned by timer.cpp
  timer_callback_t callback;
  uint32_t expiry;               // RTOS_GET_MS() of the next run
  uint32_t period;               // ms
  uint8_t autoReload;
  volatile uint8_t active;       // started, not yet expired or stopped
  uint8_t linked;                // in the active list (unlinked lazily)
} rtos_timer_t;

// Set up (or redefine) a timer; it is left stopped. Any time, task or ISR
// context, also on a started/stopped timer. The object must be static (zero
// initialized before the first use, never freed: it is linked into a list).
void timer_create(rtos_timer_t * timer, timer_callback_t callback,
                  uint32_t periodMs, bool autoReload);
// (Re)start: expires periodMs from now (restarts if already active).
// Task or ISR context.
void timer_start(rtos_timer_t * timer);
// Task or ISR context. A callback already picked for execution still runs.
void timer_stop(rtos_timer_t * timer);

// tasksStart(): after RTOS_INIT(), before any timer_start() and before
// creating any task (SIMU threads start at once)
void timer_init();
TASK_FUNCTION(timerTask);

#endif // _RTOS_TIMER_H_
