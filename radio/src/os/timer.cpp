/*
 * Software timers - shared by the TinyK and SIMU backends
 *
 * Copyright (C) OpenI6X / OpenTX
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * See timer.h. The timer task waits for a task notification with a timeout
 * equal to the time until the nearest expiry; timer_start() notifies it so
 * the wait is recomputed. No kernel (SysTick) support is needed.
 */

#include "opentx.h"

#define TIMER_WAIT_MAX_MS  0x7FFFFFFFu  // longest wait the backends handle

static timer_handle_t * timerList;      // active timers, unsorted: keep it short

TASK_DEFINE_STACK(timerStack, TIMER_STACK_SIZE);
task_handle_t timerTaskId;

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

void timer_create(timer_handle_t * timer, timer_func_t func,
                  unsigned period, bool repeat)
{
  // next/linked are left alone: zero for a fresh static object, and a timer
  // still in the active list (e.g. just stopped) is unlinked by the timer task.
  timer_lock_t lock = timerLock();
  timer->_func = func;
  timer->_period = period;
  timer->_repeat = repeat && period;        // period 0 can only be one-shot
  timer->_active = 0;                       // (re)create = stopped
  timerUnlock(lock);
}

void timer_start(timer_handle_t * timer)
{
  timer_lock_t lock = timerLock();
  timer->_expiry = time_get_ms() + timer->_period;
  timer->_active = 1;
  if (!timer->_linked) {
    timer->_next = timerList;               // insert at the head only
    timerList = timer;
    timer->_linked = 1;
  }
  timerUnlock(lock);
  task_notify_isr(&timerTaskId);            // timer task recomputes its wait
}

void timer_stop(timer_handle_t * timer)
{
  timer->_active = 0;  // byte store; unlinked by the timer task
}

// Run the expired callbacks; return the ms until the next expiry.
static uint32_t timerRun()
{
  uint32_t wait = TIMER_WAIT_MAX_MS;
  timer_lock_t lock = timerLock();
  uint32_t now = time_get_ms();
  timer_handle_t ** link = &timerList;
  while (timer_handle_t * timer = *link) {
    if (!timer->_active) {
      *link = timer->_next;                 // stopped or expired one-shot: unlink
      timer->_linked = 0;
      continue;
    }
    int32_t left = (int32_t)(timer->_expiry - now);
    if (left > 0) {
      if ((uint32_t)left < wait) {
        wait = left;
      }
      link = &timer->_next;
      continue;
    }
    if (timer->_repeat) {
      timer->_expiry += timer->_period;     // no drift
      if ((int32_t)(timer->_expiry - now) <= 0) {
        timer->_expiry = now + timer->_period;    // missed periods: skip, no burst
      }
    }
    else {
      timer->_active = 0;  // before the call: a restart meanwhile runs it again
    }
    timer_func_t callback = timer->_func;   // read under the lock
    timerUnlock(lock);
    callback();
    lock = timerLock();
    // The list may have changed (head inserts, restarts, stops): rescan.
    now = time_get_ms();
    wait = TIMER_WAIT_MAX_MS;
    link = &timerList;
  }
  timerUnlock(lock);
  return wait;
}

static void timerTask()
{
  while (true) {
#if defined(SIMU)
    if (pwrCheck() == e_power_off) {
      return;
    }
#endif
    // A timer_start() during timerRun() leaves the notification pending:
    // the wait below then returns at once and the list is scanned again.
    uint32_t wait = timerRun();
#if defined(TIMER_POLL_MS)
    if (wait > TIMER_POLL_MS) {
      wait = TIMER_POLL_MS;
    }
#endif
    (void)task_wait_notify(wait);
  }
}

void timer_init()
{
  task_create(&timerTaskId, timerTask, "Timer", timerStack, TIMER_STACK_SIZE);
}
