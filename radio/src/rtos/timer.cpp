/*
 * Software timers - shared by the TinyK, CoOS and SIMU backends
 *
 * Copyright (C) OpenI6X / OpenTX
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * See timer.h. timerTask waits on timerFlag with a timeout equal to the time
 * until the nearest expiry; timer_start() sets the flag so the wait is
 * recomputed. No kernel (SysTick) support is needed.
 */

#include "opentx.h"

#define TIMER_WAIT_MAX_MS  0x7FFFFFFFu  // longest wait the backends handle

static rtos_timer_t * timerList;        // active timers, unsorted: keep it short
static RTOS_FLAG_HANDLE timerFlag;

#if defined(SIMU)
static pthread_mutex_t timerMutex = PTHREAD_MUTEX_INITIALIZER;
typedef int timer_lock_t;
static inline timer_lock_t timerLock()
{
  pthread_mutex_lock(&timerMutex);
  return 0;
}
static inline void timerUnlock(timer_lock_t)
{
  pthread_mutex_unlock(&timerMutex);
}
#else
// task or ISR context, callers may already have IRQs disabled: save/restore
typedef uint32_t timer_lock_t;
static inline timer_lock_t timerLock()
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  return primask;
}
static inline void timerUnlock(timer_lock_t primask)
{
  __set_PRIMASK(primask);
}
#endif

void timer_init()
{
  RTOS_CREATE_FLAG(timerFlag);
}

void timer_create(rtos_timer_t * timer, timer_callback_t callback,
                  uint32_t periodMs, bool autoReload)
{
  // next/linked are left alone: zero for a fresh static object, and a timer
  // still in the active list (e.g. just stopped) is unlinked by timerTask.
  timer_lock_t lock = timerLock();
  timer->callback = callback;
  timer->period = periodMs;
  timer->autoReload = autoReload && periodMs;  // period 0 can only be one-shot
  timer->active = 0;                           // (re)create = stopped
  timerUnlock(lock);
}

void timer_start(rtos_timer_t * timer)
{
  timer_lock_t lock = timerLock();
  timer->expiry = RTOS_GET_MS() + timer->period;
  timer->active = 1;
  if (!timer->linked) {
    timer->next = timerList;          // insert at the head only
    timerList = timer;
    timer->linked = 1;
  }
  timerUnlock(lock);
  RTOS_ISR_SET_FLAG(timerFlag);       // timerTask recomputes its wait
}

void timer_stop(rtos_timer_t * timer)
{
  timer->active = 0;                  // byte store; unlinked by timerTask
}

// Run the expired callbacks; return the ms until the next expiry.
static uint32_t timerRun()
{
  uint32_t wait = TIMER_WAIT_MAX_MS;
  timer_lock_t lock = timerLock();
  uint32_t now = RTOS_GET_MS();
  rtos_timer_t ** link = &timerList;
  while (rtos_timer_t * timer = *link) {
    if (!timer->active) {
      *link = timer->next;            // stopped or expired one-shot: unlink
      timer->linked = 0;
      continue;
    }
    int32_t left = (int32_t)(timer->expiry - now);
    if (left > 0) {
      if ((uint32_t)left < wait) {
        wait = left;
      }
      link = &timer->next;
      continue;
    }
    if (timer->autoReload) {
      timer->expiry += timer->period;           // no drift
      if ((int32_t)(timer->expiry - now) <= 0) {
        timer->expiry = now + timer->period;    // missed periods: skip, no burst
      }
    }
    else {
      timer->active = 0;   // before the call: a restart meanwhile runs it again
    }
    timer_callback_t callback = timer->callback;  // read under the lock
    timerUnlock(lock);
    callback();
    lock = timerLock();
    // The list may have changed (head inserts, restarts, stops): rescan.
    now = RTOS_GET_MS();
    wait = TIMER_WAIT_MAX_MS;
    link = &timerList;
  }
  timerUnlock(lock);
  return wait;
}

TASK_FUNCTION(timerTask)
{
  while (true) {
#if defined(SIMU)
    if (pwrCheck() == e_power_off) {
      TASK_RETURN();
    }
#endif
    RTOS_CLEAR_FLAG(timerFlag);         // a timer_start() after this re-arms it
    uint32_t wait = timerRun();
#if defined(SIMU)
    (void)wait;
    wait = 1;                           // SIMU flags are dummies: poll each 1 ms
#else
    wait = (wait + RTOS_MS_PER_TICK - 1) / RTOS_MS_PER_TICK;   // ms -> ticks
#endif
    (void)RTOS_WAIT_FLAG(timerFlag, wait);
  }
}
