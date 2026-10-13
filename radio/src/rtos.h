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

#ifndef _RTOS_H_
#define _RTOS_H_

#include "definitions.h"

#ifdef __cplusplus
extern "C++" {
#endif

#if defined(SIMU)
  #include <pthread.h>
  #include <semaphore.h>

  #define SIMU_SLEEP_OR_EXIT_MS(x)       simuSleep(x)
  #define RTOS_MS_PER_TICK  1

  typedef pthread_t RTOS_TASK_HANDLE;
  typedef pthread_mutex_t RTOS_MUTEX_HANDLE;

  typedef uint32_t RTOS_FLAG_HANDLE;

  typedef sem_t * RTOS_EVENT_HANDLE;

  extern uint64_t simuTimerMicros(void);
  extern uint8_t simuSleep(uint32_t ms);

  static inline void RTOS_INIT()
  {
  }

  static inline void RTOS_START()
  {
  }

  static inline void RTOS_WAIT_MS(uint32_t x)
  {
    simuSleep(x);
  }

  static inline void RTOS_WAIT_TICKS(uint32_t x)
  {
    RTOS_WAIT_MS(x * RTOS_MS_PER_TICK);
  }

#ifdef __cplusplus
  static inline void RTOS_CREATE_MUTEX(pthread_mutex_t &mutex)
  {
    mutex = PTHREAD_MUTEX_INITIALIZER;
  }

  static inline void RTOS_LOCK_MUTEX(pthread_mutex_t &mutex)
  {
      pthread_mutex_lock(&mutex);
  }

  static inline void RTOS_UNLOCK_MUTEX(pthread_mutex_t &mutex)
  {
      pthread_mutex_unlock(&mutex);
  }

  static inline void RTOS_CREATE_FLAG(RTOS_FLAG_HANDLE flag)
  {
  }

  static inline void RTOS_SET_FLAG(RTOS_FLAG_HANDLE flag)
  {
  }

  static inline void RTOS_CLEAR_FLAG(RTOS_FLAG_HANDLE flag)
  {
  }

  static inline bool RTOS_WAIT_FLAG(RTOS_FLAG_HANDLE flag, uint32_t timeout)
  {
    simuSleep(timeout);
    return false;
  }

  #define RTOS_ISR_SET_FLAG RTOS_SET_FLAG

  template<int SIZE>
  class FakeTaskStack
  {
    public:
      FakeTaskStack()
      {
      }

      void paint()
      {
      }

      uint32_t size()
      {
        return SIZE;
      }

      uint32_t available()
      {
        return SIZE / 2;
      }
  };
  #define RTOS_DEFINE_STACK(name, size) FakeTaskStack<size> name

  #define TASK_FUNCTION(task)           void * task(void * pdata)

  inline void RTOS_CREATE_TASK(pthread_t &taskId, void * task(void *), const char * name)
  {
    pthread_create(&taskId, nullptr, task, nullptr);
#ifdef __linux__
    pthread_setname_np(taskId, name);
#endif
  }

template<int SIZE>
  inline void RTOS_CREATE_TASK(pthread_t &taskId, void * task(void *), const char * name, FakeTaskStack<SIZE> &, unsigned size = 0)
  {
    UNUSED(size);
    RTOS_CREATE_TASK(taskId, task, name);
  }

  #define TASK_RETURN()                 return nullptr

  constexpr uint32_t stackAvailable()
  {
    return 500;
  }
#endif  // __cplusplus

  // 2 ms resolution (historic, kept for SIMU timing compatibility)
  static inline uint32_t RTOS_GET_TIME(void)
  {
    return (uint32_t)(simuTimerMicros() / 2000);
  }

  static inline uint32_t RTOS_GET_MS(void)
  {
    return (uint32_t)(simuTimerMicros() / 1000);
  }
  
#elif defined(RTOS_TINYK)
  // TinyK: minimal preemptive kernel for Cortex-M0, see rtos/tinyk.h
  #include "rtos/tinyk.h"

  #define RTOS_MS_PER_TICK              1

  typedef uint8_t RTOS_TASK_HANDLE;
  typedef tk_mutex_t RTOS_MUTEX_HANDLE;
  typedef tk_flag_t RTOS_FLAG_HANDLE;

  static inline void RTOS_INIT()
  {
  }

  static inline void RTOS_START()
  {
    tk_start();
  }

  static inline void RTOS_WAIT_MS(uint32_t x)
  {
    tk_delay(x);
  }

  static inline void RTOS_WAIT_TICKS(uint32_t x)
  {
    tk_delay(x);
  }

  // Priority = creation order (first created = highest priority).
  #define RTOS_CREATE_TASK(taskId, task, name, stackStruct, stackSize) \
                                        taskId = tk_create(task, stackStruct.stack, stackSize)

#ifdef __cplusplus
  static inline void RTOS_CREATE_MUTEX(tk_mutex_t & mutex)
  {
    tk_mutex_init(&mutex);
  }

  static inline void RTOS_LOCK_MUTEX(tk_mutex_t & mutex)
  {
    tk_mutex_lock(&mutex);
  }

  static inline void RTOS_UNLOCK_MUTEX(tk_mutex_t & mutex)
  {
    tk_mutex_unlock(&mutex);
  }

  static inline void RTOS_CREATE_FLAG(tk_flag_t & flag)
  {
    flag.set = 0;
    flag.waiters = 0;
  }

  static inline void RTOS_SET_FLAG(tk_flag_t & flag)
  {
    tk_flag_set(&flag);
  }

  static inline void RTOS_ISR_SET_FLAG(tk_flag_t & flag)
  {
    tk_flag_set(&flag);
  }

  static inline void RTOS_CLEAR_FLAG(tk_flag_t & flag)
  {
    flag.set = 0;
  }

  // returns true on timeout
  static inline bool RTOS_WAIT_FLAG(tk_flag_t & flag, uint32_t timeout)
  {
    return !tk_flag_wait(&flag, timeout);
  }
#endif  // __cplusplus

  static inline uint32_t getStackAvailable(void * address, uint32_t size)
  {
    uint32_t * array = (uint32_t *)address;
    uint32_t i = 0;
    while (i < size && array[i] == TK_STACK_PAINT) {
      i++;
    }
    return i * 4;
  }

  extern int _estack;
  extern int _main_stack_start;
  static inline uint32_t stackSize()
  {
    return ((unsigned char *)&_estack - (unsigned char *)&_main_stack_start) / 4;
  }

  static inline uint32_t stackAvailable()
  {
    return getStackAvailable(&_main_stack_start, stackSize());
  }

#ifdef __cplusplus
  template<int SIZE>
  class TaskStack
  {
    public:
      TaskStack()
      {
      }

      // no paint(): tk_create() paints the stack

      uint32_t size()
      {
        return SIZE * 4;
      }

      uint32_t available()
      {
        return getStackAvailable(stack, SIZE);
      }

      uint32_t stack[SIZE];
  };
#endif // __cplusplus

  static inline uint32_t RTOS_GET_TIME(void)
  {
    return tk_ticks;
  }

  static inline uint32_t RTOS_GET_MS(void)
  {
    return tk_ticks;
  }

  #if defined(TK_IDLE_STATS)
  #define RTOS_HAS_IDLE_STATS 1
  #define RTOS_IDLE_UNITS_PER_TICK (TK_IDLE_CLOCK_HZ / 1000u * RTOS_MS_PER_TICK)
  static inline uint32_t RTOS_GET_IDLE_TIME(void)
  {
    return tk_idle_time;
  }
  #define RTOS_HAS_TASK_STATS 1
  static inline uint32_t RTOS_GET_TASK_TIME(RTOS_TASK_HANDLE id)
  {
    return tk_task_time[id];     // same units as RTOS_GET_IDLE_TIME
  }
  #endif

  #define RTOS_DEFINE_STACK(name, size) TaskStack<size> __ALIGNED(8) name // stack must be aligned to 8 bytes otherwise printf for %f does not work!

  #define TASK_FUNCTION(task)           void task(void * pdata)
  #define TASK_RETURN()                 return

#else // no RTOS
  static inline void RTOS_START()
  {
  }

  static inline void RTOS_WAIT_MS(unsigned x)
  {
  }

  static inline void RTOS_WAIT_TICKS(unsigned x)
  {
  }
#endif  // RTOS type

#if defined(__cplusplus) && (defined(SIMU) || defined(RTOS_TINYK))
  // Software timers: one implementation for TinyK and SIMU (callbacks run in
  // timerTask, see rtos/timer.h)
  #include "rtos/timer.h"

  typedef rtos_timer_t RTOS_TIMER_HANDLE;

  static inline void RTOS_CREATE_TIMER(RTOS_TIMER_HANDLE & timer, timer_callback_t callback, uint32_t periodMs, bool autoReload)
  {
    timer_create(&timer, callback, periodMs, autoReload);
  }

  static inline void RTOS_START_TIMER(RTOS_TIMER_HANDLE & timer)
  {
    timer_start(&timer);
  }

  static inline void RTOS_STOP_TIMER(RTOS_TIMER_HANDLE & timer)
  {
    timer_stop(&timer);
  }
#endif

#ifdef __cplusplus
}
#endif

#endif // _RTOS_H_
