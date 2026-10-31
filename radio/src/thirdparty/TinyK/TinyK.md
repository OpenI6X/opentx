# TinyK: kernel and OS layer documentation (OpenI6X / PCBI6X)

TinyK is the preemptive priority kernel used by OpenI6X on the FlySky i6X (STM32F072, Cortex-M0, 16 KB RAM, 128 KB flash). It replaced CoOS. On top of it, a thin OS layer in `radio/src/os/` provides the EdgeTX `os/` API (tasks, notifications, mutexes, time, sleep, software timers), so OpenTX code does not call the kernel directly.

This document describes the code on branch `janek/tinyk` at `73d87a1ba`. The sizes were measured on that commit with the I6X release build (GCC 14.2.1, `-Os`, LTO).

---

## Contents

1. [Design goals and feature set](#1-design-goals-and-feature-set)
2. [Source layout and layering](#2-source-layout-and-layering)
3. [Build configuration](#3-build-configuration)
4. [Kernel concepts](#4-kernel-concepts)
5. [Scheduling and context switch](#5-scheduling-and-context-switch)
6. [Interrupts and critical sections](#6-interrupts-and-critical-sections)
7. [Kernel API reference (`tinyk.h`)](#7-kernel-api-reference-tinykh)
8. [OS layer API reference (`os/`)](#8-os-layer-api-reference-os)
9. [Software timers (`os/timer.h`)](#9-software-timers-ostimerh)
10. [Integration in OpenTX on PCBI6X](#10-integration-in-opentx-on-pcbi6x)
11. [Diagnostics: stacks and CPU usage](#11-diagnostics-stacks-and-cpu-usage)
12. [Memory footprint](#12-memory-footprint)
13. [Rules, limitations and pitfalls](#13-rules-limitations-and-pitfalls)
14. [SIMU backend](#14-simu-backend)
15. [Testing](#15-testing)
16. [How to: add a task, a timer, a deferred ISR job](#16-how-to-add-a-task-a-timer-a-deferred-isr-job)

---

## 1. Design goals and feature set

TinyK implements exactly what OpenTX on PCBI6X needs:

| Feature | TinyK |
|---|---|
| Tasks | Up to `TK_MAX_TASKS` (default 3) static tasks, plus 1 built-in idle task |
| Priorities | Fixed. **Priority = creation order**: the first task created has the highest priority. No time slicing. |
| Tick | 1 ms SysTick; `tk_ticks` counts ms since start |
| Delay | `tk_delay(ticks)` |
| Signalling | One binary **task notification** per task. `tk_notify()` works from a task or an ISR; `tk_notify_wait(timeout)` clears the bit on return. |
| Mutex | Blocking and non-recursive. No priority inheritance; see [§13](#13-rules-limitations-and-pitfalls). |
| Stack diagnostics | Stacks are painted at creation for high-water marks. An optional canary check runs every tick (`TK_STACK_CHECK`). |
| CPU accounting | Optional per-task and idle run time (`TK_IDLE_STATS`) |
| Context switch | PendSV at the lowest NVIC level; tasks run on PSP, ISRs on MSP |
| Not provided | Dynamic tasks, task deletion, queues, event groups, counting semaphores, round-robin, tickless idle |

Everything else that used to need an RTOS object is built from these primitives in the OS layer. Software timers are an example: one timer task, notified by `timer_start()`.

---

## 2. Source layout and layering

```
radio/src/
├── thirdparty/TinyK/
│   ├── tinyk.h            kernel API, configuration macros
│   └── tinyk.c            scheduler, SysTick, PendSV, notifications, mutex
├── os/                    EdgeTX-style OS abstraction (file names and API as in EdgeTX)
│   ├── task.h             task_create, notifications, mutex, stack/CPU queries
│   ├── task_tinyk.h/.cpp  TinyK backend (handles, TASK_DEFINE_STACK, MSP stack info)
│   ├── task_native.h/.cpp SIMU backend (pthreads)
│   ├── time.h             time_get_ms()
│   ├── time_tinyk.cpp / time_native.cpp
│   ├── sleep.h            sleep_ms()
│   ├── sleep_tinyk.cpp / sleep_native.cpp
│   ├── timer.h            software timers API
│   ├── timer.cpp          timer list and timer task (shared by both backends)
│   └── timer_tinyk.h / timer_native.h   TIMER_STACK_SIZE (+ TIMER_POLL_MS in SIMU)
├── rtos.h                 RTOS_START() → tk_start()
└── tasks.cpp / tasks.h    OpenTX tasks (mixer, menus), stack sizes, tasksStart()
```

How the layers depend on each other:

```
 OpenTX code (tasks.cpp, telemetry, sbus, audio, gui, ...)
        │  task_*, mutex_*, timer_*, time_get_ms, sleep_ms     (only os/ headers)
        ▼
 os/ layer ──────────────► timer.cpp (timer task, built on task_* + time_get_ms)
        │  tk_*
        ▼
 TinyK kernel (tinyk.c) ──► CMSIS: SysTick, SCB->ICSR (PendSV), NVIC, PRIMASK, PSP
```

The `os/` headers are kept C-compatible, because FatFs (`ff.c`) includes them. `timer_*` are C++ functions; do not declare them `extern "C"`.

---

## 3. Build configuration

TinyK is the only RTOS. CMake always defines `RTOS_TINYK`, and `RTOS_DIR` points to `thirdparty/TinyK`.

| CMake option | Define | Default | Effect |
|---|---|---|---|
| `TINYK_STACK_CHECK` | `TK_STACK_CHECK` | OFF | SysTick checks each task's lowest stack word (the canary) every tick and calls `tk_stack_overflow()` once per task. |
| `TINYK_IDLE_STATS` | `TK_IDLE_STATS` → `TASK_CPU_STATS` | OFF | Per-task and idle run-time counters, plus the CPU line on the statistics screen |

Compile-time macros in `tinyk.h`, which can be overridden with `-D`:

| Macro | Default | Meaning |
|---|---|---|
| `TK_MAX_TASKS` | 3 | User task slots: mixer, timer, menus. Must be ≤ 8, because the ready/sleep/notify sets are `uint8_t` bitmasks. |
| `TK_IDLE_STACK_WORDS` | 24 | Idle task stack: 16-word context plus margin. Idle only executes `WFI`. |
| `TK_STACK_PAINT` | `0x55555555` | Stack paint pattern |
| `TK_IDLE_CLOCK()` / `TK_IDLE_CLOCK_HZ` | `TIM7->CNT`, 2 MHz | Free-running 16-bit clock used by `TK_IDLE_STATS` |

Stack sizes are in 32-bit **words**:

| Define | Where | Words | Bytes | Analyzed worst case |
|---|---|---|---|---|
| `MIXER_STACK_SIZE` | `tasks.h` | 200 | 800 | 132 words |
| `MENUS_STACK_SIZE` | `tasks.h` | 256 | 1024 | 172 words |
| `TIMER_STACK_SIZE` | `os/timer_tinyk.h` | 128 | 512 | 112 words (AFHDS2A GPS telemetry path with one recursion) |
| `TK_IDLE_STACK_WORDS` | `tinyk.h` | 24 | 96 | 16 words (context only) |
| Main stack (MSP) | linker script `_Main_Stack_Size` | 256 | 1024 | Used by ISRs and by code before `tk_start()`; not painted, so not measured |

---

## 4. Kernel concepts

### 4.1 Tasks and priorities

A task is a `void fn(void)` with a static, 8-byte aligned stack, registered with `tk_create()` before `tk_start()`. Its id is its creation index, starting at 0, and that id is also its priority: **lower id = higher priority**. Two tasks never share a priority, so the scheduler is a pure "highest-ready-wins" selection with no time slicing. A task runs until it blocks, or until a higher-priority task becomes ready.

The idle task uses the slot after the last user task (`id == tk_count`). It runs `__WFI()` in a loop and is selected when no user task is ready.

If a task function returns, the task is parked forever by `tk_task_exit()`, which is the LR of the initial frame.

### 4.2 Task states

The kernel keeps state as bitmasks (bit *i* = task *i*):

| Mask | Meaning |
|---|---|
| `tk_ready` | Runnable |
| `tk_sleeping` | A wake-up time (`tcb.wake`) is armed: delay or notify timeout |
| `tk_notified` | A notification is pending |
| `tk_nwaiting` | Blocked in `tk_notify_wait()` |
| `mutex.waiters` | Blocked on that mutex (per mutex) |

```
                 tk_create()
                      │
                      ▼
   ┌──────────────► READY ◄────────────────────────────────┐
   │                  │ highest ready id                    │
   │                  ▼                                     │
   │               RUNNING ──── preempted by higher prio ───┘
   │                  │
   │   tk_delay(t)    │   tk_notify_wait(t)     tk_mutex_lock() on a taken mutex
   │        ▼         │          ▼                       ▼
   │    SLEEPING      │   WAIT_NOTIFY (+ sleeping)   WAIT_MUTEX
   │        │         │          │                       │
   └─ tick expires ───┴─ tk_notify() / timeout ── tk_mutex_unlock() (wakes all waiters)
```

### 4.3 Time

- SysTick runs at `SystemCoreClock / 1000`, so 1 tick = 1 ms.
- `tk_ticks` wraps after 49.7 days. All expiry comparisons use signed differences (`(int32_t)(now - wake) >= 0`), so they are wrap-safe for waits up to 2³¹ − 1 ms.
- A delay of *n* ticks expires on the *n*-th tick interrupt after the call. The real wait is therefore between *n* − 1 and *n* ms.

### 4.4 Before the scheduler starts

Until `tk_start()` runs, the code runs on the MSP in `main()`:
- `tk_delay()` / `sleep_ms()`, `tk_mutex_lock()` / `unlock()` are **no-ops**.
- `tk_notify_wait()` returns 0.
- `tk_notify()` only sets the pending bit, which is kept.

This lets initialization code that uses mutexes or `sleep_ms()` run unchanged before the start, but `sleep_ms()` does **not** wait there.

---

## 5. Scheduling and context switch

### 5.1 `tk_schedule()`

`tk_schedule()` is called with IRQs disabled after every state change. It scans `tk_ready` from bit 0 upwards; the first set bit is the task to run, and if none is set the idle slot is chosen. If that differs from the current task, it sets `tk_next` and pends PendSV (`SCB->ICSR = PENDSVSET`). The scan is O(`tk_count`), at most 3 iterations here.

With `TK_IDLE_STATS`, the same function also charges the time since its last call to the task or idle that was running. It is called at least once per tick, so the 16-bit 2 MHz clock cannot wrap in between.

### 5.2 PendSV

PendSV and SysTick run at NVIC level 3, the lowest on the M0. A switch therefore happens only after all other ISRs have finished. When several state changes happen in a burst, they collapse into one switch.

```
Task stack (PSP) at switch-out:
 high  ┌──────────┐
       │ xPSR     │  ┐
       │ PC       │  │
       │ LR       │  │ hardware-stacked by exception entry (8 words)
       │ R12      │  │
       │ R3..R0   │  ┘
       │ R11..R8  │  ┐ software-saved by PendSV_Handler (8 words)
 low   │ R7..R4   │  ┘ ◄── tcb.sp
       └──────────┘
```

`PendSV_Handler`, a naked asm function, works like this:
1. Store `PSP - 32` in `tk_cur->sp`.
2. Push R4–R7, then R8–R11 through R4–R7. ARMv6-M `stm` only handles low registers.
3. Set `tk_cur = tk_next` and restore R8–R11, then R4–R7.
4. Set PSP and return with `EXC_RETURN = 0xFFFFFFFD` (thread mode, PSP).

Cost: about 59 cycles of handler code plus about 32 cycles of exception entry and exit, i.e. about 90 cycles, or about 2 µs at 48 MHz with flash wait states.

### 5.3 Initial frame and the first switch

`tk_create()` paints the whole stack with `TK_STACK_PAINT`, then builds a fake exception frame at the 8-byte-aligned top:
- xPSR = Thumb bit;
- PC = `fn`;
- LR = `tk_task_exit`;
- R0–R3 and R12 = 0;
- 8 words for R4–R11.

Painting has to happen before the frame is written, otherwise the frame would be overwritten.

`tk_start()` does the following:
1. Builds the idle task's frame.
2. Sets PendSV and SysTick to level 3 and starts SysTick.
3. Points `tk_cur` at a throw-away boot TCB whose "stack" is `tk_boot_scratch[8]`, so the first PendSV has somewhere to save the boot context.
4. Calls `tk_schedule()` and enables IRQs.

PendSV then switches to task 0 and never returns to `main()`. From then on the MSP is used only by ISRs.

---

## 6. Interrupts and critical sections

### 6.1 Critical sections

The kernel protects its state with **PRIMASK**, using `__disable_irq()` / `__enable_irq()`. On ARMv6-M there is no BASEPRI, so a kernel critical section blocks **all** interrupts, including level-0 ISRs. Kernel critical sections are short: a few bit operations plus `tk_schedule()`. The longest is `SysTick_Handler`, which loops over the sleeping tasks and, with `TK_STACK_CHECK`, over the canaries.

| Function | Context | Critical section style |
|---|---|---|
| `tk_notify()` | Task or ISR, also with IRQs already disabled | Save and restore PRIMASK |
| `timer_start()`, `timer_create()` (`timerLock`) | Task or ISR, also with IRQs disabled | Save and restore PRIMASK |
| `SysTick_Handler` | ISR | Plain disable/enable: PRIMASK is always 0 on entry |
| `tk_delay()`, `tk_notify_wait()`, `tk_mutex_*()` | **Task only** | Plain disable/enable. The switch happens at `__enable_irq()`. |

### 6.2 Calling the kernel from ISRs

These calls are ISR-safe: `tk_notify()` (= `task_notify_isr()`), `timer_start()`, `timer_stop()`, `timer_create()`, `time_get_ms()`, `tk_started_get()`.

These calls must **never** be made from an ISR: `tk_delay()` / `sleep_ms()`, `tk_notify_wait()` / `task_wait_notify()`, `tk_mutex_lock()` / `mutex_lock()`, `tk_mutex_unlock()`.

All NVIC levels may call the ISR-safe functions. PendSV is at the lowest level, so a wake-up from an ISR takes effect when the last nested ISR exits.

### 6.3 I6X interrupt levels

The M0 implements 2 priority bits, so the effective level is `value & 3`, with 0 the highest:

| Level | ISRs |
|---|---|
| 0 | USART2 (CRSF telemetry, RTO end-of-frame trigger) |
| 1 | DMA1 channels 4–7 (telemetry TX) |
| 2 | TIM15, TIM16 (module timers), EXTI2_3 (A7105) |
| 3 | TIM17 (mixer scheduler trigger), USART1 (AUX / SBUS), USB, **SysTick**, **PendSV** |

Task priorities: **mixer > timer > menus > idle**.

---

## 7. Kernel API reference (`tinyk.h`)

OpenTX code should use the `os/` layer ([§8](#8-os-layer-api-reference-os)). The kernel API is listed for completeness and for porting.

| Function | Context | Description |
|---|---|---|
| `uint8_t tk_create(void (*fn)(void), uint32_t *stack, uint32_t words)` | Before `tk_start()` | Paints the stack, builds the initial frame and marks the task ready. Returns the task id (= priority), or `0xFF` if started already or if all `TK_MAX_TASKS` slots are used. |
| `void tk_start(void)` | `main()`, once | Creates idle, starts SysTick and switches to the highest-priority task. Does not return. |
| `uint8_t tk_started_get(void)` | Any | 1 after `tk_start()` |
| `void tk_delay(uint32_t ticks)` | Task | Blocks for `ticks` ms. `0`, or before start: returns at once (no yield). |
| `void tk_notify(uint8_t task)` | Task or ISR | Sets the task's pending bit and wakes it if it is waiting. Invalid ids are ignored. |
| `uint8_t tk_notify_wait(uint32_t ticks)` | Task | Returns 1 at once if a notification is pending; otherwise blocks up to `ticks` ms. Returns 1 if notified, 0 on timeout. Always clears the pending bit. `ticks = 0` only polls. |
| `uint32_t tk_stack_free(const uint32_t *stack, uint32_t words)` | Any | Bytes from the bottom of the stack whose paint is still intact (high-water mark) |
| `void tk_mutex_init(tk_mutex_t *m)` | Any | Sets `owner = 0xFF` (free). **Required**: a zeroed mutex would be "owned by task 0". |
| `void tk_mutex_lock(tk_mutex_t *m)` | Task | Blocks while the mutex is owned. Not recursive: a second lock by the owner deadlocks. |
| `void tk_mutex_unlock(tk_mutex_t *m)` | Task | Releases the mutex and makes **all** waiters ready; the highest priority gets it. The owner is not checked. |
| `void tk_stack_overflow(uint8_t task)` | SysTick (`TK_STACK_CHECK`) | Weak hook. The default sets bit `task` in `tk_overflow`. |

| Variable | Description |
|---|---|
| `volatile uint32_t tk_ticks` | ms since `tk_start()` |
| `volatile uint8_t tk_overflow` | Tasks whose canary was found overwritten (`TK_STACK_CHECK`) |
| `volatile uint32_t tk_idle_time`, `tk_task_time[TK_MAX_TASKS]` | Run time in `TK_IDLE_CLOCK` units (`TK_IDLE_STATS`). Free running, wraps. |

---

## 8. OS layer API reference (`os/`)

The `os/` layer uses the file names and function names of EdgeTX's `radio/src/os/`. Deliberate differences from EdgeTX:
- `task_create()` has **no priority argument**; priority = creation order.
- Timer callbacks are `void cb()`, with no argument.
- Task notifications are exposed as `task_notify()` / `task_notify_isr()` / `task_wait_notify()`.
- `task_get_cpu_time()` / `task_get_idle_time()` exist with `TASK_CPU_STATS`.

### 8.1 Tasks (`os/task.h`)

```cpp
TASK_DEFINE_STACK(name, words);   // uint32_t name[words], 8-byte aligned

void     task_create(task_handle_t* h, task_func_t func, const char* name,
                     void* stack, unsigned stack_size /* words */);
unsigned task_get_stack_usage(task_handle_t* h);   // bytes never used (as in EdgeTX: free bytes)
unsigned task_get_stack_size(task_handle_t* h);    // bytes
bool     scheduler_is_running();
```

- `task_handle_t` (TinyK) holds `_stack`, `_stack_size` and `_id`. A static, zeroed handle with `_stack == NULL` means "not created". `task_notify*()` and the stack/CPU queries ignore a handle that was never created.
- If `tk_create()` fails (too many tasks, or called after start), `task_create()` returns silently and leaves the handle "not created". Raise `TK_MAX_TASKS` when adding a task.
- `name` is ignored by TinyK.
- MSP information (TinyK only): `stackSize()` returns words; `stackAvailable()` returns the bytes whose paint is still intact. **On I6X the startup code (`targets/flysky/startup_stm32f072.s`) does not paint the main stack**, so `stackAvailable()` reads arbitrary RAM and its value is meaningless. The I6X statistics screen has that line commented out; only the optional CLI prints it.

### 8.2 Task notifications

```cpp
void task_notify(task_handle_t* h);       // task context
void task_notify_isr(task_handle_t* h);   // ISR context (same code on TinyK)
bool task_wait_notify(uint32_t timeout_ms);  // calling task; true = notified, false = timeout
```

A notification behaves like a binary semaphore owned by the task, the same as a FreeRTOS direct-to-task notification used with `ulTaskNotifyTake(pdTRUE, …)`:
- Notifications are **not counted**. Several notifies before a wait produce **one** wake-up.
- A notify that arrives while the task is busy stays pending, so the next wait returns at once and no event is lost.

### 8.3 Mutexes

```cpp
void mutex_create(mutex_handle_t* h);   // mandatory before first use
bool mutex_lock(mutex_handle_t* h);     // always returns true (no timeout)
void mutex_unlock(mutex_handle_t* h);
```

Mutexes in use: `mixerMutex`, which protects the mixer data used by the mixer and menus tasks, and `audioMutex`, only with `VOICE`.

### 8.4 Time and sleep

```cpp
uint32_t time_get_ms();      // = tk_ticks
void     sleep_ms(uint32_t ms);   // = tk_delay(ms); no-op before the scheduler starts
```

For sub-millisecond timing, use the hardware timers (`getTmr2MHz()`), not the OS.

---

## 9. Software timers (`os/timer.h`)

### 9.1 Model

Software timers replace the old dedicated telemetry task and the TIM14 5 ms interrupt. They work like FreeRTOS timers:
- all timer **callbacks run in one task**, the *timer task* (`timerTaskId`, name `"Timer"`);
- that task is created by `timer_init()` between the mixer and the menus task, so it runs **below the mixer and above menus**.

```cpp
struct timer_handle_t { ... };     // must be a static object (zero-initialized, never freed)
typedef void (*timer_func_t)();

void timer_create(timer_handle_t* h, timer_func_t func, unsigned period_ms, bool repeat);
bool timer_is_created(timer_handle_t* h);   // func != nullptr
bool timer_is_active(timer_handle_t* h);    // started and not yet expired or stopped
void timer_start(timer_handle_t* h);        // (re)start: expires period_ms from now
void timer_stop(timer_handle_t* h);
void timer_init();                          // creates the timer task (tasksStart())
```

### 9.2 Semantics

| Aspect | Behaviour |
|---|---|
| `timer_create` | Sets callback, period and repeat, and leaves the timer **stopped**. It may be called again to redefine a timer, even an active one. Task or ISR context. |
| `period = 0` | Always one-shot, and expires "now": the callback runs as soon as the timer task gets the CPU. This is the deferred-ISR-work idiom ([§16](#16-how-to-add-a-task-a-timer-a-deferred-isr-job)), equivalent to EdgeTX `async_call_isr()`. |
| `timer_start` on an active timer | Restarts it: the new expiry is now + period. |
| `timer_start` from an ISR | Allowed. It notifies the timer task, which recomputes its wait. |
| `timer_stop` | A single byte store. The timer is removed from the list lazily by the timer task. A callback that is already executing still finishes. |
| One-shot expiry | The timer is marked inactive **before** the callback runs. A `timer_start()` during the callback (or from an ISR meanwhile) therefore schedules it again, and the trigger is not lost. |
| Periodic | `expiry += period`, so there is no drift. If periods were missed (timer task blocked), the next expiry is now + period: missed periods are skipped, not run in a burst. |
| Callback context | The timer task, with the list lock released. Callbacks may call any `timer_*` function and `task_notify()`. They should not block for long (`sleep_ms()`, mutex waits), because that delays every other timer. |
| Order | Expired timers run one at a time. After each callback the list is rescanned from the head, because callbacks may change it. The list is unsorted and new timers are inserted at the head; it is meant to stay short. |

### 9.3 Timer task loop

```
loop:
  wait = timerRun()            // run the expired callbacks, return ms until the next expiry
  task_wait_notify(wait)       // woken by timeout or by timer_start()
```

`timerRun()` holds a PRIMASK lock only while it walks the list, and releases it around each callback. If `timer_start()` is called while callbacks are running, its notification stays pending, so the next wait returns at once and the list is scanned again.

### 9.4 Timers on PCBI6X

| Timer | Period | Started by | Callback work |
|---|---|---|---|
| `_timer10ms` (`tasks.cpp`) | 10 ms periodic | `tasksStart()` | `buzzerWakeup()`, `dfplayerWakeup()` (with `DFPLAYER`), `per10ms()` |
| `telemetryTimer` (`telemetry.cpp`) | 2 ms periodic | `telemetryStart()` | `telemetryWakeup()` while pulses are running |
| `_poll_frame_timer` (`telemetry.cpp`) | 0, one-shot | `telemetryFrameTrigger_ISR()`: USART2 RTO (CRSF end of frame) or the A7105 ISR (AFHDS2A packet) | `_poll_frame()`: copy the CRSF DMA FIFO and parse it, or parse the FlySky packet; telemetry mirror |
| `trainerSbusFrameTimer` (`sbus.cpp`) | 0, one-shot | AUX USART RTO ISR (end of SBUS frame) | `sbusFrameReceivedCb()`: copy and decode the 25-byte SBUS frame |

---

## 10. Integration in OpenTX on PCBI6X

### 10.1 Start sequence

```
main()
 ├─ boardInit()                       hardware, IRQs mostly not yet enabled
 └─ tasksStart()
     ├─ timer_create(trainerSbusFrameTimer)   before any ISR can start it
     ├─ mixerTaskInit()
     │    ├─ mixerSchedulerInit(), mutex_create(&mixerMutex)
     │    ├─ task_create(mixer)               id 0 → highest priority
     │    └─ mixerSchedulerStart()
     ├─ timer_init()                          task_create(timer): id 1
     ├─ timer10msStart()                      create + start _timer10ms
     ├─ task_create(menus)                    id 2 → lowest user priority
     └─ RTOS_START() → tk_start()             never returns
                                       menusTask() then runs opentxInit()
```

### 10.2 Tasks

| Id / priority | Task | Stack | Woken by | Work |
|---|---|---|---|---|
| 0 (highest) | Mixer (`mixerTask`) | 200 words | Mixer scheduler ISR (TIM17) → `task_notify_isr(&mixerTaskId)`, 5 ms timeout | `doMixerCalculations()`, pulses, periodic updates, under `mixerMutex` |
| 1 | Timer (`timerTask`) | 128 words | `timer_start()` (ISRs, tasks), or the next expiry | Timer callbacks ([§9.4](#94-timers-on-pcbi6x)), including the telemetry parsing |
| 2 | Menus (`menusTask`) | 256 words | `sleep_ms()`, 50 ms period minus run time | `opentxInit()`, then `perMain()`: GUI, LCD, EEPROM, audio queue |
| — (lowest) | Idle | 24 words | — | `WFI` |

### 10.3 Event paths (ISR → task)

```
TIM17 (mixer scheduler) ─task_notify_isr─────────────────────────► Mixer task
USART2 RTO (CRSF) ─────telemetryFrameTrigger_ISR → timer_start ───► Timer task: _poll_frame()
A7105 EXTI (AFHDS2A) ──telemetryFrameTrigger_ISR → timer_start ───► Timer task: _poll_frame()
USART1 RTO (SBUS) ─────timer_start(trainerSbusFrameTimer) ────────► Timer task: sbusFrameReceivedCb()
SysTick ───────────────delays, timeouts expire ───────────────────► any task
```

Telemetry parsing runs in the timer task, so a long menus pass (LCD refresh, model load, EEPROM write) cannot delay it, and the 128 B CRSF DMA FIFO cannot overrun. It is still preempted by the mixer, so mixer timing is not affected.

---

## 11. Diagnostics: stacks and CPU usage

### 11.1 Stack high-water marks

Every task stack is painted with `0x55555555` in `tk_create()`. `task_get_stack_usage(h)` returns the number of bytes at the bottom that were never touched. The main stack (MSP) is **not** painted on I6X, so there is no high-water mark for ISR stack usage ([§8.1](#81-tasks-ostaskh)). Painting `_main_stack_start` up to the current SP early in `Reset_Handler` would enable it.

The statistics/debug screen shows "free stack" as **menus / mixer / timer** in bytes.

### 11.2 Stack overflow check (`TINYK_STACK_CHECK`)

Every tick, SysTick compares each task's lowest stack word with the paint value. On the first mismatch per task it calls `tk_stack_overflow(task)`. That function is weak; the default only sets the task's bit in `tk_overflow`. Nothing in OpenTX reads `tk_overflow` yet, so inspect it with a debugger, or override the hook (for example to show an alert or reset).

This is a canary check: it detects an overflow only after it has happened, and only if the bottom word was actually written.

### 11.3 CPU usage (`TINYK_IDLE_STATS`)

- `tk_schedule()` charges elapsed `TIM7` 2 MHz time to the task or idle that ran. ISR time is charged to whichever task or idle was interrupted.
- The statistics screen takes a 1-second snapshot and shows:
  - total **CPU %** and idle ms;
  - per-task share in ‰, in the same order: menus / mixer / timer.

---

## 12. Memory footprint

I6X release build at `73d87a1ba`:

| Build | Flash | RAM |
|---|---|---|
| Default | 122,912 B | 13,904 B |
| `TINYK_IDLE_STATS=YES TINYK_STACK_CHECK=YES` | 123,468 B (+556, including the screen code) | 13,976 B (+72) |

Breakdown of the default build, from `nm -S`:

| Part | Flash | RAM |
|---|---|---|
| Kernel code: `tk_schedule` 92, `tk_notify_wait` 132, `tk_notify` 92, `tk_delay` 88, `SysTick_Handler` 88, `PendSV_Handler` 72, mutex lock/unlock 112, `tk_task_exit` 48, idle 4 | ≈ 730 B | — |
| OS layer: `task_create` (incl. `tk_create`) 144, `task_get_stack_usage` 40; small wrappers inlined | ≈ 185 B | — |
| Timers: `timerTask` (incl. `timerRun`) 136, `timer_start` 84; `timer_create` inlined at its call sites | ≈ 220 B | `timerList` 4, `timerTaskId` 8 |
| Kernel data: TCBs 4 × 8, boot TCB and scratch 40, state bytes and pointers ≈ 20 | — | ≈ 90 B |
| Idle stack | — | 96 B |
| Task stacks: mixer 800, menus 1024, timer 512 | — | 2,336 B |
| Main stack (MSP, linker) | — | 1,024 B |

With `TK_IDLE_STATS`, each TCB is still 8 B, but `tk_task_time[3]`, `tk_idle_time` and `tk_stat_t0` are added. With `TK_STACK_CHECK`, each TCB grows to 12 B (`base`).

For comparison, from the earlier CoOS-removal analysis on the same target: switching from CoOS to TinyK saved about 1.9 KB of flash and about 150 B of RAM before any stack tuning. The RAM gain grew to about 2 KB once task stacks were resized to analyzed values.

---

## 13. Rules, limitations and pitfalls

1. **Priority = creation order.** Changing the order of `task_create()` / `timer_init()` in `tasksStart()` changes priorities. Keep mixer → timer → menus.
2. **`TK_MAX_TASKS` must cover every task.** It defaults to 3. A fourth `task_create()`, such as the `VOICE` audio task, fails silently and leaves the handle "not created". The hard upper limit is 8, because of the `uint8_t` masks.
3. **No priority inheritance.** A high-priority task blocked on a mutex held by a low-priority task can be delayed by a medium-priority task. The only mutex shared between priority levels is `mixerMutex`, used by mixer (0) and menus (2). The timer task (1) does not take it, but it can run while menus holds the mutex and the mixer waits. The inversion is therefore bounded by the timer callbacks' run time, which is short. Keep timer callbacks short and keep them away from `mixerMutex`.
4. **Mutexes are not recursive**, unlock does not check the owner, and `mutex_create()` is mandatory.
5. **Notifications are binary.** Never use them to count events. Keep the count in the data (a FIFO, a counter) and use the notification only to wake the task.
6. **Cross-task flags must be `volatile`.** With LTO, a plain global read in a loop can be folded into a constant. This happened once and removed the whole mixer body; see the `s_pulses_paused` fix.
7. **Never block in an ISR.** That includes `sleep_ms()`, `task_wait_notify()` and `mutex_lock()`. Use `task_notify_isr()` or `timer_start()` to defer work.
8. **Do not block in timer callbacks.** They share one task. A `sleep_ms()` in a callback delays all timers, including telemetry parsing.
9. **Timer objects must be static.** They are linked into the timer list and never freed.
10. **`sleep_ms()` before `tk_start()` does not wait.** Code in `main()` before `tasksStart()` must busy-wait on a hardware timer if it needs a delay.
11. **No yield.** `tk_delay(0)` returns at once. A task that never blocks starves all lower-priority tasks, including idle, and with it the CPU statistics.
12. **Kernel critical sections mask all IRQs**, even level 0. Never add long loops inside them.
13. **Stack sizes are in words** for `TASK_DEFINE_STACK`, `task_create` and `TIMER_STACK_SIZE`. `task_get_stack_usage()` / `task_get_stack_size()` return bytes.

---

## 14. SIMU backend

In SIMU builds (`os/*_native.*`):
- tasks are pthreads; `TASK_DEFINE_STACK` is a dummy;
- mutexes are `pthread_mutex_t`;
- notifications use a flag with atomic builtins;
- `time_get_ms()` / `sleep_ms()` map to the simulator clock;
- the timer code (`timer.cpp`) is shared, with a pthread mutex as the lock and `TIMER_POLL_MS = 1`, so the timer task polls every millisecond and notices simulator shutdown (`pwrCheck()`);
- `RTOS_START()` is empty.

Note: the I6X SIMU build is currently broken upstream on 64-bit hosts, for reasons unrelated to TinyK.

---

## 15. Testing

From the review (`report/janek_tinyk_review/`):

- **`timer_test/`:** host test of the real `os/timer.cpp` against a fake time source. 16 cases, all PASS:
  - one-shot with period 0 and with 10 ms;
  - periodic without drift, with missed periods skipped;
  - restart, stop before expiry, stop + start before unlink;
  - restart from its own callback; a callback that starts or stops another timer;
  - nearest expiry wins; tick wrap-around; periodic with period 0 forced to one-shot;
  - re-create while stopped-but-linked, while active, from its own callback, and in the middle of the list;
  - `timer_init()` creates the timer task.
- **`qemu_test/`:** `tinyk.c` on an emulated Cortex-M0 (QEMU), built with `TK_STACK_CHECK`. Result: PASS.
  - **Phase 0:** kernel calls before `tk_start()` are harmless no-ops.
  - **Phase 1:** a 3 s stress run of the mixer/timer/menus pattern: notifications from an ISR and from tasks, a mutex held across ticks, preemption.
  - **Phase 2:** stack paint on create, `tk_stack_free()`, no false canary alarm, and a simulated overflow is detected.
- Firmware-level checks of the telemetry and SBUS paths that run in the timer task: `crsf_frame_test/`, `dmafifo_test/`, `dmafifo_bench/`, `mirror_test/`, `sbus_test/`.

---

## 16. How to: add a task, a timer, a deferred ISR job

### Add a task

```cpp
// tasks.h
#define FOO_STACK_SIZE 128                 // words; size it from the -fstack-usage / callgraph analysis

// tasks.cpp
task_handle_t fooTaskId;
TASK_DEFINE_STACK(fooStack, FOO_STACK_SIZE);

static void fooTask()
{
  while (true) {
    if (task_wait_notify(100)) {           // woken by task_notify(_isr)(&fooTaskId)
      // handle the event
    }
    // 100 ms housekeeping
  }
}

// tasksStart(): position = priority
task_create(&fooTaskId, fooTask, "Foo", fooStack, FOO_STACK_SIZE);
```

Then raise `TK_MAX_TASKS`, which costs 8 B of TCB per task, and add the task to the stack line on the statistics screen.

### Add a periodic timer (preferred over a new task for light periodic work)

```cpp
static timer_handle_t fooTimer;            // static!
static void fooTimerCb() { /* short, non-blocking */ }

if (!timer_is_created(&fooTimer)) {
  timer_create(&fooTimer, fooTimerCb, 20 /*ms*/, true);
}
timer_start(&fooTimer);
```

### Defer work from an ISR

```cpp
static timer_handle_t fooJob;              // timer_create(&fooJob, fooProcess, 0, false) at init,
                                           // before the ISR is enabled
extern "C" void FOO_IRQHandler()
{
  // acknowledge the hardware, capture the minimum
  timer_start(&fooJob);                    // fooProcess() runs soon in the timer task
}
```

A deferred job runs at timer-task priority: below the mixer, above menus. Use it for parsing or decoding work that would be too long for an ISR. Several triggers before the job runs collapse into one run, so the job must process everything that is pending (for example, empty the whole FIFO).
