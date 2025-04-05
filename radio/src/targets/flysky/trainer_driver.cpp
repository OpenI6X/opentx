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

void init_trainer_capture() {
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin        = TRAINER_IN_GPIO_PIN;
  GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;  // erfly6: GPIO_PuPd_UP
  GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  LL_GPIO_Init(TRAINER_GPIO, &GPIO_InitStruct);

  extmoduleTimerStart();
}

void stop_trainer_capture()
{
  // disable PPM input capture
  CLEAR_BIT(EXTMODULE_TIMER->DIER, TIM_DIER_CC1IE);
  CLEAR_BIT(EXTMODULE_TIMER->CCER, TIM_CCER_CC1E);

  // Keep timer running because PPM OUT uses the same timer
  // NVIC_DisableIRQ(EXTMODULE_TIMER_IRQn);
}

#if defined(SBUS_TRAINER)
int sbusGetByte(uint8_t * byte)
{
  switch (currentTrainerMode) {
#if defined(AUX_SERIAL_USART)
    case TRAINER_MODE_MASTER_BATTERY_COMPARTMENT:
      return auxSerialRxFifo.pop(*byte);
#endif
    default:
      return false;
  }
}
#endif
