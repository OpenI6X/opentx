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

#pragma once

#include "f0/stm32f0_serial.h"
#include "board.h"

namespace flysky_serial {

using stm32f0_serial::NoPin;
using stm32f0_serial::Usart;

// SystemInit selects PCLK for USART1/2 and configures HSI48 with AHB/APB
// prescalers at /1. USART3/4 are wired to PCLK on the STM32F072.
constexpr Usart aux1{
  USART1_BASE, PERI2_FREQUENCY,
  STM32F0_GPIO_AF_PIN(GPIOA, 9, LL_GPIO_AF_1),
  STM32F0_GPIO_AF_PIN(GPIOA, 10, LL_GPIO_AF_1),
  USART1_IRQn, 7, LL_DMA_CHANNEL_3, 0,
};

constexpr Usart telemetry{
  USART2_BASE, PERI1_FREQUENCY,
  STM32F0_GPIO_AF_PIN(GPIOD, 5, LL_GPIO_AF_0),
  STM32F0_GPIO_AF_PIN(GPIOA, 15, LL_GPIO_AF_1),
  USART2_IRQn, 0, LL_DMA_CHANNEL_5, LL_DMA_CHANNEL_4,
};
constexpr IRQn_Type telemetryDmaTxIrq = DMA1_Channel4_5_6_7_IRQn;
constexpr uint8_t telemetryDmaTxPriority = 1;

#if defined(AUX3_SERIAL)
constexpr Usart aux3{
  USART3_BASE, PERI1_FREQUENCY,
  STM32F0_GPIO_AF_PIN(GPIOC, 10, LL_GPIO_AF_1), NoPin,
  USART3_4_IRQn, 7, 0, 0,
};
#endif

#if defined(AUX4_SERIAL)
constexpr Usart aux4{
  USART4_BASE, PERI1_FREQUENCY,
  NoPin, STM32F0_GPIO_AF_PIN(GPIOC, 11, LL_GPIO_AF_0),
  USART3_4_IRQn, 7, LL_DMA_CHANNEL_6, 0,
};
#endif

} // namespace flysky_serial
