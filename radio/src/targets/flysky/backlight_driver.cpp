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

void backlightInit()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = BACKLIGHT_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(BACKLIGHT_GPIO, &GPIO_InitStructure);
  GPIO_PinAFConfig(BACKLIGHT_GPIO, BACKLIGHT_GPIO_PinSource, BACKLIGHT_GPIO_AF);

  BACKLIGHT_TIMER->ARR = 100;
  BACKLIGHT_TIMER->PSC = BACKLIGHT_TIMER_FREQ / 50000 - 1; // 20us * 100 = 2ms => 500Hz
  BACKLIGHT_TIMER->CCMR2 = BACKLIGHT_CCMR2; // PWM
  BACKLIGHT_TIMER->CCER = BACKLIGHT_CCER;
  BACKLIGHT_COUNTER_REGISTER = 100;
  BACKLIGHT_TIMER->EGR = 0;
  BACKLIGHT_TIMER->CR1 = TIM_CR1_CEN;  // Counter enable

  // std
  GPIO_InitTypeDef gpio_init;
  gpio_init.GPIO_Mode  = GPIO_Mode_OUT;
  gpio_init.GPIO_OType = GPIO_OType_PP;
  gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
  gpio_init.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  gpio_init.GPIO_Pin   = BACKLIGHT_STD_GPIO_PIN;
  GPIO_Init(BACKLIGHT_STD_GPIO, &gpio_init);
}

void backlightEnable(uint8_t level)
{
  BACKLIGHT_COUNTER_REGISTER = /*100 -*/ level;
  BACKLIGHT_TIMER->CR1 = TIM_CR1_CEN;

  // std
  if (level == 0) { // inverted
    GPIO_SetBits(BACKLIGHT_STD_GPIO, BACKLIGHT_STD_GPIO_PIN);
  } else {
    GPIO_ResetBits(BACKLIGHT_STD_GPIO, BACKLIGHT_STD_GPIO_PIN);
  }
}

void backlightDisable()
{
  BACKLIGHT_COUNTER_REGISTER = 100;
  BACKLIGHT_TIMER->CR1 &= ~TIM_CR1_CEN;          // solves very dim light with backlight off

  // std
  GPIO_ResetBits(BACKLIGHT_STD_GPIO, BACKLIGHT_STD_GPIO_PIN);
}

uint8_t isBacklightEnabled()
{
  return BACKLIGHT_COUNTER_REGISTER != 100 || GPIO_ReadInputDataBit(BACKLIGHT_STD_GPIO, BACKLIGHT_STD_GPIO_PIN) != 0;
}
