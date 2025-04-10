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
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  // Configure EXT Module Power pin as output
  GPIO_InitStruct.Pin        = EXTMODULE_PWR_GPIO_PIN;
  GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Pull       = LL_GPIO_PULL_UP;
  // Reset EXT Module Power pin (set to low)
  LL_GPIO_ResetOutputPin(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN);
  LL_GPIO_Init(EXTMODULE_PWR_GPIO, &GPIO_InitStruct);

  pwrOn();
}

void pwrOn() {
}

void pwrOff() {
#if defined(PWR_BUTTON_PRESS)
  GPIO_ResetBits(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN); // power off ext module
  
  for (;;) {
    // Wait for switch off
  }
#endif
}

#define PWR_PRESS_DURATION_MIN 200 // 2s

/**
 * i6X dont have a dedicated power trigger so use CANCEL (KEY_EXIT) to emulate it
 * or on SWITCH mode ignore it completelly, saving data is done in main screen popup.
 */
uint32_t pwrPressed() {
#if defined(PWR_BUTTON_PRESS)
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
#else // SWITCH
 return 1;
#endif // PWR_BUTTON_PRESS
}
