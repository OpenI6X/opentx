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

#define KEY_ADC_MAX   4095
#define KEY_ADC_STEP  KEY_ADC_MAX / 3
#define KEY_ADC_VAL_1 KEY_ADC_STEP * 1 // 1/3
#define KEY_ADC_VAL_2 KEY_ADC_STEP * 2 // 2/3

#define KEY_MATRIX_LINES 4
#define KEY_MATRIX_COLUMNS 3
static const uint16_t columns[] = {KEYS_MATRIX_R1_PIN, KEYS_MATRIX_R2_PIN, KEYS_MATRIX_R3_PIN};
static const uint16_t lines[] = {KEYS_MATRIX_L1_PIN, KEYS_MATRIX_L2_PIN, KEYS_MATRIX_L3_PIN, KEYS_MATRIX_L4_PIN};
/*
  KEY_RIGHT,
  KEY_LEFT,
*/

/*
      R1	    R2	        R3
  L1	Roll R	Throttle U	Down
  L2	Roll L	Throttle D	Up
  L3	Pitch U	Yaw R	    OK
  L4	Pitch D	Yaw L	    Cancel*/

const uint8_t keysMap[] = {
    TRM_RH_UP, TRM_RH_DWN, TRM_RV_UP, TRM_RV_DWN,
    TRM_LV_UP, TRM_LV_DWN, TRM_LH_UP, TRM_LH_DWN,
    KEY_DOWN, KEY_UP, KEY_ENTER, KEY_EXIT};

uint32_t scanMatrix(uint32_t columnStart, uint32_t columnEnd)
{
  uint32_t result = 0;
  uint8_t column = 0;
  uint8_t line = 0;
  uint16_t index = columnStart * KEY_MATRIX_LINES;
  for (column = columnStart; column <= columnEnd; column++)
  {
    //set to low
    KEYS_MATRIX_COLUMNS_GPIO->BRR = columns[column];
    //read lines
    for (line = 0; line < KEY_MATRIX_LINES; line++)
    {
      if ((KEYS_MATRIX_LINES_GPIO->IDR & lines[line]) == 0)
      {
        result |= (1 << ((uint32_t)keysMap[index]));
      }
      index++;
    }
    //set to hight
    KEYS_MATRIX_COLUMNS_GPIO->BSRR = columns[column];
  }
  return result;
}

uint32_t readKeys()
{
  uint32_t result = scanMatrix(2, 2);
  //bind active low
  if ((KEYS_BIND_GPIO->IDR & KEYS_BIND_PIN) == 0)
  {
    // if (!result)
    // {
      result |= 1 << KEY_RIGHT;
    // }
    // else
    // {
    //   //bind as shift
    //   if (result & (1 << KEY_DOWN))
    //   {
    //     result &= ~(1 << KEY_DOWN);
    //     result |= 1 << KEY_LEFT;
    //   }
    //   if (result & (1 << KEY_UP))
    //   {
    //     result &= ~(1 << KEY_UP);
    //     result |= 1 << KEY_RIGHT;
    //   }
    // }
  }
  return result;
}

uint32_t readTrims()
{
  return scanMatrix(0, 1);
}

uint8_t trimDown(uint8_t idx)
{
  return (readTrims() & (1 << (TRM_BASE + idx))) ? 1 : 0;
}

bool keyDown()
{
  return readKeys() || readTrims();
}

/* TODO common to ARM */
void readKeysAndTrims()
{
  uint8_t i;
  uint8_t index = 0;
  uint32_t keys_input = readKeys();

  for (i = 0; i < TRM_BASE; i++)
  {
    keys[index++].input(keys_input & (1 << i));
  }

  uint32_t trims_input = readTrims();
  for (i = TRM_BASE; i <= TRM_LAST; i++)
  {
    keys[index++].input(trims_input & (1 << i));
  }

  if ((keys_input || trims_input) && (g_eeGeneral.backlightMode & e_backlight_mode_keys))
  {
    // on keypress turn the light on
    resetBacklightTimeout();
  }
}

uint8_t keyState(uint8_t index)
{
  return keys[index].state();
}

#define ADD_2POS_CASE(x) \
  case SW_S ## x ## 0: \
    xxx = SWITCHES_GPIO_REG_ ## x  & SWITCHES_GPIO_PIN_ ## x ; \
    break; \
  case SW_S ## x ## 2: \
    xxx = ~SWITCHES_GPIO_REG_ ## x  & SWITCHES_GPIO_PIN_ ## x ; \
    break

#if !defined(BOOT)
uint32_t switchState(uint8_t index)
{
  uint32_t xxx = 0;

  uint8_t pos = index % 3;    // 0, 1, 2
  uint8_t sw_num = index / 3; // 0, 1, 2, 3
  uint8_t adc_num = sw_num + 4;

  if (index < SW_SE0) { // SA, SB, SC, SD
    if (sw_num > 1) {
      adc_num += 2; // skip the 2 pots
    }

    uint16_t value = adcValues[adc_num];
    if (((value <= KEY_ADC_VAL_1) && (pos == 0))
        || ((value >  KEY_ADC_VAL_1) && (pos == 1) && (value <= KEY_ADC_VAL_2))
        || ((value >  KEY_ADC_VAL_2) && (pos == 2)))
    {
      xxx = 1 << index;
    }
  } else { // SE, SF
    switch (index) {
      ADD_2POS_CASE(E);
      ADD_2POS_CASE(F);
      default:
        break;
    }
  }

  // TRACE("switch idx %d sw_num %d value %d pos %d xxx %d", index, sw_num, value, pos, xxx);
  return xxx;
}
#endif

void keysInit()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = KEYS_LINES_PINS;
  GPIO_Init(KEYS_MATRIX_LINES_GPIO, &GPIO_InitStructure);

  // SE, SF
#if defined(KEYS_GPIOD_PINS)
  INIT_KEYS_PINS(GPIOD);
#elif defined(KEYS_GPIOC_PINS)
  INIT_KEYS_PINS(GPIOC);
#endif

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = KEYS_COLUMNS_PINS;
  GPIO_Init(KEYS_MATRIX_COLUMNS_GPIO, &GPIO_InitStructure);
  //set high
  KEYS_MATRIX_COLUMNS_GPIO->BSRR = KEYS_COLUMNS_PINS;
}
