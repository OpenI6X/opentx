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

uint16_t isqrt32(uint32_t n)
{
  uint16_t c = 0x8000;
  uint16_t g = 0x8000;

  for (;;) {
    if ((uint32_t)g*g > n)
      g ^= c;
    c >>= 1;
    if(c == 0)
      return g;
    g |= c;
  }
}

/*
  Division by 10 and rounding or fixed point arithmetic values

  Examples:
    value -> result
    105 ->  11
    104 ->  10
   -205 -> -21
   -204 -> -20
*/

#if defined(STM32F0)

int fast_div10(uint32_t num)
{
  return (uint32_t)(((uint64_t)num * 0xCCCCCCCDULL) >> 35);
}

int fast_div100(uint32_t num)
{
  return (uint32_t)(((uint64_t)num * 0x51EB851FULL) >> 37);
}

int fast_div1000(uint32_t num)
{
  return (uint32_t)(((uint64_t)num * 0x10624DD3ULL) >> 38);
}

int div_and_round10(int num)
{
  if (num >= 0) return fast_div10((uint32_t)(num + 5));
  else return -fast_div10((uint32_t)(-num + 5));
}

int div_and_round100(int num)
{
  if (num >= 0) return fast_div100((uint32_t)(num + 50));
  else return -fast_div100((uint32_t)(-num + 50));
}

int div_and_round1000(int num)
{
  if (num >= 0) return fast_div1000((uint32_t)(num + 500));
  else return -fast_div1000((uint32_t)(-num + 500));
}

int div_and_round(int num, int den)
{
  // compiler is automatically inlining those cases
  if (den == 10) return div_and_round10(num);
  if (den == 1000) return div_and_round1000(num);
  if (den == 100) return div_and_round100(num);
  if (den == 0) return 0;
  if (num >= 0) num += den / 2;
  else num -= den / 2;
  return num / den;
}
#endif

// djb2 hash algorithm
uint32_t hash(const void * ptr, uint32_t size)
{
  const uint8_t * data = (const uint8_t *)ptr;
  uint32_t hash = 5381;
  for (uint32_t i=0; i<size; i++) {
    hash = ((hash << 5) + hash) + data[i]; /* hash * 33 + c */
  }
  return hash;
}
