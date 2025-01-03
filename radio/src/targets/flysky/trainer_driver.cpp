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
  GPIO_PinAFConfig(TRAINER_GPIO, TRAINER_IN_GPIO_PinSource, TRAINER_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = TRAINER_IN_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  // erfly6: GPIO_PuPd_UP
  GPIO_Init(TRAINER_GPIO, &GPIO_InitStructure);
}

void stop_trainer_capture()
{
  // disable PPM input capture
  // nothing to do because it would stop EXTMODULE_TIMER
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
