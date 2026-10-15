/*
 * Software timers (EdgeTX os/timer.h names) - TinyK and SIMU backends
 *
 * Copyright (C) OpenI6X / OpenTX
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * FreeRTOS-like: static timer objects with a callback, one-shot or
 * auto-reload, (re)started / stopped from task or ISR context. Callbacks run
 * in the timer task (created by timer_init(), like the FreeRTOS timer
 * daemon), never in an ISR.
 *
 * A one-shot timer with period 0 runs its callback as soon as possible after
 * timer_start(): use it to defer work out of an ISR (e.g. telemetry parsing).
 *
 * Callbacks run one after another in one task:
 *   - keep each well below 1 ms (they delay each other and telemetry),
 *   - never block, never take mixerMutex (menus may hold it for a model load).
 * Times are in ms, resolution is one RTOS tick (TinyK 1 ms).
 * Differences to EdgeTX: callbacks take no argument, timer_create() has no
 * name, the functions return void (they cannot fail).
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "task.h"

// OS specific configuration (timer task stack, ...)
#if defined(SIMU)
  #include "timer_native.h"
#elif defined(RTOS_TINYK)
  #include "timer_tinyk.h"
#endif

typedef void (*timer_func_t)();

struct timer_handle_t {
  timer_handle_t* _next;          // active list, owned by timer.cpp
  timer_func_t    _func;
  uint32_t        _expiry;        // time_get_ms() of the next run
  uint32_t        _period;        // ms
  uint8_t         _repeat;
  volatile uint8_t _active;       // started, not yet expired or stopped
  uint8_t         _linked;        // in the active list (unlinked lazily)
};

// Set up (or redefine) a timer; it is left stopped. Any time, task or ISR
// context, also on a started/stopped timer. The object must be static (zero
// initialized before the first use, never freed: it is linked into a list).
void timer_create(timer_handle_t* h, timer_func_t func, unsigned period,
                  bool repeat);

bool timer_is_created(timer_handle_t * h);

// (Re)start: expires period ms from now (restarts if already active).
// Task or ISR context.
void timer_start(timer_handle_t* h);
// Task or ISR context. A callback already picked for execution still runs.
void timer_stop(timer_handle_t* h);

// Creates the timer task. TinyK priority = creation order: call it in
// tasksStart() where the timer task priority belongs (after the mixer task,
// before the menus task). Timers may be created and started before.
void timer_init();
extern task_handle_t timerTaskId;   // debug screen: stack / CPU usage
