/*
 * TinyK - minimal preemptive priority kernel for Cortex-M0 (ARMv6-M)
 *
 * Copyright (C) OpenI6X / OpenTX
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * Exactly what OpenTX on PCBI6X needs, nothing more:
 *   - up to TK_MAX_TASKS static tasks + 1 idle task
 *     priority = creation order (first created = highest priority)
 *   - 1 ms tick (SysTick), tk_delay()
 *   - manual-reset event flag, settable from ISR, wait with timeout
 *   - blocking, non-recursive mutex (no priority inheritance: with the two
 *     OpenTX tasks there is no medium-priority task that could cause inversion)
 *   - optional stack-overflow canary check (TK_STACK_CHECK)
 *
 * Context switch: PendSV at the lowest NVIC level, tasks on PSP, ISRs on MSP.
 * All kernel calls are ISR safe except tk_delay/tk_flag_wait/tk_mutex_*
 * (task context only). Before tk_start() the blocking calls are no-ops.
 */

#ifndef _TINYK_H_
#define _TINYK_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TK_MAX_TASKS
#define TK_MAX_TASKS        3     // user tasks: mixer, menus (+1 spare)
#endif
#ifndef TK_IDLE_STACK_WORDS
#define TK_IDLE_STACK_WORDS 24    // 16 words context + margin; idle calls nothing
#endif
#define TK_STACK_PAINT      0x55555555u   // tk_create() paints;

typedef struct {
  volatile uint8_t set;           // manual reset: cleared by the user
  volatile uint8_t waiters;       // bitmask of waiting tasks
} tk_flag_t;

typedef struct {
  volatile uint8_t owner;         // 0xFF = free
  volatile uint8_t waiters;       // bitmask of blocked tasks
} tk_mutex_t;

extern volatile uint32_t tk_ticks;      // ms since tk_start(), wraps after 49.7 days
extern volatile uint8_t  tk_overflow;   // bitmask of tasks whose stack canary died
#if defined(TK_IDLE_STATS)
// Run time of idle and of each task is measured with a free-running 16-bit
// up-counter sampled at every scheduler call (at least once per 1 ms tick, so
// it cannot wrap in between). ISR time is charged to the interrupted task or
// idle. Default: the OpenTX 2 MHz timer (TIM7 on PCBI6X).
#if !defined(TK_IDLE_CLOCK)
#define TK_IDLE_CLOCK()     ((uint16_t)TIM7->CNT)
#define TK_IDLE_CLOCK_HZ    2000000u
#endif
extern volatile uint32_t tk_idle_time;  // idle time in TK_IDLE_CLOCK units
extern volatile uint32_t tk_task_time[TK_MAX_TASKS];  // per task (tk_create id), same units
#endif

uint8_t tk_create(void (*fn)(void *), uint32_t * stack, uint32_t words);  // 0xFF on error; paints the stack
void    tk_start(void) __attribute__((noreturn));
void    tk_delay(uint32_t ticks);

void    tk_flag_set(tk_flag_t * f);                   // task or ISR context
uint8_t tk_flag_wait(tk_flag_t * f, uint32_t ticks);  // 1 = flag set, 0 = timeout

void    tk_mutex_init(tk_mutex_t * m);
void    tk_mutex_lock(tk_mutex_t * m);
void    tk_mutex_unlock(tk_mutex_t * m);

// called from SysTick when a canary is overwritten (TK_STACK_CHECK only).
// Default (weak) implementation only records the task in tk_overflow.
void    tk_stack_overflow(uint8_t task);

#ifdef __cplusplus
}
#endif

#endif // _TINYK_H_
