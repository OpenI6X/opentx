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
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin        = BACKLIGHT_GPIO_PIN;
  GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
  LL_GPIO_Init(BACKLIGHT_GPIO, &GPIO_InitStruct);

  BACKLIGHT_TIMER->ARR = 100;
  BACKLIGHT_TIMER->PSC = BACKLIGHT_TIMER_FREQ / 50000 - 1; // 20us * 100 = 2ms => 500Hz
  BACKLIGHT_TIMER->CCMR2 = BACKLIGHT_CCMR2; // PWM
  BACKLIGHT_TIMER->CCER = BACKLIGHT_CCER;
  BACKLIGHT_COUNTER_REGISTER = 100;
  BACKLIGHT_TIMER->EGR = 0;
  BACKLIGHT_TIMER->CR1 = TIM_CR1_CEN;  // Counter enable

  // std
  LL_GPIO_InitTypeDef gpio_init = {0};
  gpio_init.Pin        = BACKLIGHT_STD_GPIO_PIN;
  gpio_init.Mode       = LL_GPIO_MODE_OUTPUT;
  gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  gpio_init.Pull       = LL_GPIO_PULL_NO;
  LL_GPIO_Init(BACKLIGHT_STD_GPIO, &gpio_init);
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
