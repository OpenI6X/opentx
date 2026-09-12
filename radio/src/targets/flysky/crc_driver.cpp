/*
 * Copyright (C) OpenI6X
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
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

#include "stm32f0xx_crc.h"
#include "crc_driver.h"

uint8_t crc8(const uint8_t * ptr, uint32_t len) {
  CRC->INIT = CRC8_INIT_VAL;
  CRC->POL = CRC8_POL_D5;
  CRC->CR = CRC_PolSize_8;
  CRC->CR |= CRC_CR_RESET;
  for (uint32_t i = 0; i < len; i++) {
    *(__IO uint8_t*)(CRC_BASE) = (*ptr++);
  }
  return (uint8_t)(CRC->DR);
}

uint8_t crc8_BA(const uint8_t * ptr, uint32_t len) {
  CRC->INIT = CRC8_INIT_VAL;
  CRC->POL = CRC8_POL_BA;
  CRC->CR = CRC_PolSize_8;
  CRC->CR |= CRC_CR_RESET;
  for (uint32_t i = 0; i < len; i++) {
    *(__IO uint8_t*)(CRC_BASE) = (*ptr++);
  }
  return (uint8_t)(CRC->DR);
}

// Used by FLYSKY_GIMBAL
uint16_t crc16(const uint8_t * ptr, uint32_t len) {
  CRC->INIT = CRC16_INIT_VAL;
  CRC->POL = CRC16_POL_1021;
  CRC->CR = CRC_PolSize_16;
  CRC->CR |= CRC_CR_RESET;
  for (uint32_t i = 0; i < len; i++) {
    *(__IO uint8_t*)(CRC_BASE) = (*ptr++);
  }
  return (uint16_t)(CRC->DR);
}
