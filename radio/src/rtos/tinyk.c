/*
 * TinyK - minimal preemptive priority kernel for Cortex-M0 (ARMv6-M)
 *
 * Copyright (C) OpenI6X / OpenTX
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * See tinyk.h for the feature set.
 */

#include "tinyk.h"
#include "stm32f0xx.h"   // CMSIS core: SysTick, SCB, NVIC, __disable_irq ...

#define TK_NONE 0xFF

typedef struct {
  uint32_t * sp;         // must stay first: used by PendSV_Handler
  uint32_t   wake;       // tick at which a delay / timeout expires
#if defined(TK_STACK_CHECK)
  uint32_t * base;       // lowest stack word = canary
#endif
} tk_tcb_t;

static tk_tcb_t   tk_tcb[TK_MAX_TASKS + 1];     // last used slot = idle
static uint8_t    tk_count;
static uint8_t    tk_started;
static uint32_t   tk_idle_stack[TK_IDLE_STACK_WORDS] __attribute__((aligned(8)));
static uint32_t   tk_boot_scratch[8];            // throw-away save area for the first switch
static tk_tcb_t   tk_boot_tcb;

volatile uint32_t tk_ticks;
volatile uint8_t  tk_overflow;
static volatile uint8_t tk_ready;     // bit i -> task i runnable
static volatile uint8_t tk_sleeping;  // bit i -> task i has a timeout armed
static uint8_t    tk_cur_idx;         // task selected to run (idle = tk_count)

tk_tcb_t * volatile tk_cur;           // used by PendSV_Handler
tk_tcb_t * volatile tk_next;

// select the highest-priority ready task; IRQs must be disabled
static void tk_schedule(void);

static void tk_task_exit(void)
{
  // a task function returned: park it forever
  __disable_irq();
  tk_ready &= ~(1u << tk_cur_idx);
  tk_sleeping &= ~(1u << tk_cur_idx);
  tk_schedule();
  __enable_irq();
  for (;;);
}

static uint32_t * tk_frame(void (*fn)(void *), uint32_t * stack, uint32_t words)
{
  uint32_t * sp = (uint32_t *)((uint32_t)(stack + words) & ~7u);   // AAPCS: 8-byte aligned
  *--sp = 0x01000000u;               // xPSR (Thumb bit)
  *--sp = (uint32_t)fn & ~1u;        // PC (bit0 must be clear in the frame)
  *--sp = (uint32_t)tk_task_exit;    // LR
  sp -= 5;                           // R12, R3, R2, R1, R0 (= arg = NULL)
  for (int i = 0; i < 5; i++) sp[i] = 0;
  sp -= 8;                           // R4..R11 (software-saved part)
  return sp;
}

static void tk_idle(void * arg)
{
  (void)arg;
  for (;;) {
    __WFI();
  }
}

uint8_t tk_create(void (*fn)(void *), uint32_t * stack, uint32_t words)
{
  if (tk_started || tk_count >= TK_MAX_TASKS) {
    return TK_NONE;
  }
  uint8_t id = tk_count++;
  tk_tcb[id].sp = tk_frame(fn, stack, words);
#if defined(TK_STACK_CHECK)
  tk_tcb[id].base = stack;
#endif
  tk_ready |= (1u << id);
  return id;
}

// select the highest-priority ready task; IRQs must be disabled
static void tk_schedule(void)
{
  uint8_t r = tk_ready, i = 0;
  while (i < tk_count && !(r & (1u << i))) {
    i++;                             // lowest index = highest priority
  }
  // i == tk_count -> idle slot
  if (i != tk_cur_idx) {
    tk_cur_idx = i;
    tk_next = &tk_tcb[i];
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
  }
}

void tk_start(void)
{
  tk_tcb[tk_count].sp = tk_frame(tk_idle, tk_idle_stack, TK_IDLE_STACK_WORDS);

  NVIC_SetPriority(PendSV_IRQn, 3);  // lowest: switch only when no ISR is active
  NVIC_SetPriority(SysTick_IRQn, 3);
  SysTick_Config(SystemCoreClock / 1000);

  __disable_irq();
  tk_boot_tcb.sp = &tk_boot_scratch[8];
  tk_cur = &tk_boot_tcb;
  __set_PSP((uint32_t)&tk_boot_scratch[8]);
  tk_cur_idx = TK_NONE;
  tk_started = 1;
  tk_schedule();
  __enable_irq();                    // PendSV fires here and never returns
  for (;;);
}

void tk_delay(uint32_t ticks)
{
  if (!ticks || !tk_started) {
    return;
  }
  __disable_irq();
  uint8_t bit = 1u << tk_cur_idx;
  tk_tcb[tk_cur_idx].wake = tk_ticks + ticks;
  tk_sleeping |= bit;
  tk_ready &= ~bit;
  tk_schedule();
  __enable_irq();                    // switch happens here
}

__attribute__((weak)) void tk_stack_overflow(uint8_t task)
{
  tk_overflow |= (1u << task);
}

void SysTick_Handler(void)
{
  __disable_irq();
  uint32_t now = ++tk_ticks;
  uint8_t s = tk_sleeping;
  for (uint8_t i = 0; s; i++, s >>= 1) {
    if ((s & 1) && (int32_t)(now - tk_tcb[i].wake) >= 0) {
      tk_sleeping &= ~(1u << i);
      tk_ready |= (1u << i);
    }
  }
#if defined(TK_STACK_CHECK)
  for (uint8_t i = 0; i < tk_count; i++) {
    if (*tk_tcb[i].base != TK_STACK_PAINT && !(tk_overflow & (1u << i))) {
      tk_stack_overflow(i);
    }
  }
#endif
  tk_schedule();
  __enable_irq();
}

void tk_flag_set(tk_flag_t * f)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  f->set = 1;
  uint8_t w = f->waiters;
  if (w) {
    f->waiters = 0;
    tk_sleeping &= ~w;
    tk_ready |= w;
    if (tk_started) {
      tk_schedule();
    }
  }
  __set_PRIMASK(primask);
}

uint8_t tk_flag_wait(tk_flag_t * f, uint32_t ticks)
{
  if (!tk_started) {
    return f->set;
  }
  __disable_irq();
  if (!f->set && ticks) {
    uint8_t bit = 1u << tk_cur_idx;
    f->waiters |= bit;
    tk_tcb[tk_cur_idx].wake = tk_ticks + ticks;
    tk_sleeping |= bit;
    tk_ready &= ~bit;
    tk_schedule();
    __enable_irq();                  // switch happens here
    __disable_irq();
    f->waiters &= ~bit;              // timeout path
  }
  uint8_t r = f->set;
  __enable_irq();
  return r;
}

void tk_mutex_init(tk_mutex_t * m)
{
  m->owner = TK_NONE;
  m->waiters = 0;
}

void tk_mutex_lock(tk_mutex_t * m)
{
  if (!tk_started) {
    return;                          // single-threaded before start
  }
  __disable_irq();
  while (m->owner != TK_NONE) {
    uint8_t bit = 1u << tk_cur_idx;
    m->waiters |= bit;
    tk_ready &= ~bit;
    tk_schedule();
    __enable_irq();                  // switch happens here
    __disable_irq();
  }
  m->owner = tk_cur_idx;
  __enable_irq();
}

void tk_mutex_unlock(tk_mutex_t * m)
{
  if (!tk_started) {
    return;
  }
  __disable_irq();
  m->owner = TK_NONE;
  uint8_t w = m->waiters;
  if (w) {
    m->waiters = 0;
    tk_ready |= w;
    tk_schedule();
  }
  __enable_irq();
}

// Context switch: save R4-R11 of tk_cur on its PSP, restore tk_next.
__attribute__((naked)) void PendSV_Handler(void)
{
  __asm volatile(
    " mrs   r0, psp            \n"
    " sub   r0, r0, #32        \n"
    " ldr   r2, =tk_cur        \n"
    " ldr   r1, [r2]           \n"
    " str   r0, [r1]           \n"   // tk_cur->sp = psp - 32
    " stmia r0!, {r4-r7}       \n"
    " mov   r4, r8             \n"
    " mov   r5, r9             \n"
    " mov   r6, r10            \n"
    " mov   r7, r11            \n"
    " stmia r0!, {r4-r7}       \n"
    " ldr   r3, =tk_next       \n"
    " ldr   r1, [r3]           \n"
    " str   r1, [r2]           \n"   // tk_cur = tk_next
    " ldr   r0, [r1]           \n"   // r0 = tk_next->sp
    " add   r0, r0, #16        \n"
    " ldmia r0!, {r4-r7}       \n"
    " mov   r8, r4             \n"
    " mov   r9, r5             \n"
    " mov   r10, r6            \n"
    " mov   r11, r7            \n"
    " sub   r0, r0, #32        \n"
    " ldmia r0!, {r4-r7}       \n"
    " add   r0, r0, #16        \n"
    " msr   psp, r0            \n"
    " ldr   r0, =0xFFFFFFFD    \n"   // return to thread mode, PSP
    " bx    r0                 \n"
    " .ltorg                   \n"
  );
}
