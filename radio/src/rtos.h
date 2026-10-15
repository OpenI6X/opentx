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

// The OS API is in os/ (task.h, time.h, sleep.h, timer.h), as in EdgeTX.
// Only the scheduler start is left here, as in EdgeTX.

#include <stdint.h>

#if defined(SIMU)
  extern uint64_t simuTimerMicros(void);
  extern uint8_t simuSleep(uint32_t ms);

  #define SIMU_SLEEP_OR_EXIT_MS(x)       simuSleep(x)

  static inline void RTOS_START()
  {
  }
#elif defined(RTOS_TINYK)
  #include "tinyk.h"

  static inline void RTOS_START()
  {
    tk_start();
  }
#endif

#endif // _RTOS_H_
