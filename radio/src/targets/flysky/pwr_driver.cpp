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

#include "opentx.h"

void pwrInit() {
  GPIO_InitTypeDef GPIO_InitStructure;

  // Init EXT Module PWR
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_PWR_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

  GPIO_ResetBits(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN);
  GPIO_Init(EXTMODULE_PWR_GPIO, &GPIO_InitStructure);

  pwrOn();
}

void pwrOn() {
}

void pwrOff() {
  GPIO_ResetBits(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN);
  
  for (;;) {
    // Wait for switch off
  }
}

#define PWR_PRESS_DURATION_MIN 200 // 2s

/**
 * i6X dont have a dedicated power trigger
 * so use CANCEL (KEY_EXIT) to emulate it.
 */
uint32_t pwrPressed() {
  static tmr10ms_t pwr_trigger_time = 0;

  if ((readKeys() & (1 << KEY_EXIT))) {
    if (pwr_trigger_time == 0) {
      pwr_trigger_time = get_tmr10ms();
    }
    if (get_tmr10ms() - pwr_trigger_time > PWR_PRESS_DURATION_MIN) {
      return 1;
    }
  } else {
    pwr_trigger_time = 0;
  }
  return 0;
}
