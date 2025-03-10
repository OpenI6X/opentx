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

#if defined(MENU_DIAG_ANAS_KEYS)
void menuRadioDiagAnalogs(event_t event)
{
// TODO enum
  #define ANAS_ITEMS_COUNT 1

  SIMPLE_SUBMENU(STR_MENU_RADIO_ANALOGS, HEADER_LINE+ANAS_ITEMS_COUNT);

  for (uint32_t i=0; i<NUM_STICKS+NUM_POTS+NUM_SLIDERS; i++) {
#if (NUM_STICKS+NUM_POTS+NUM_SLIDERS) > 9
    coord_t y = MENU_HEADER_HEIGHT + 1 + (i/3)*FH;
    const uint8_t x_coord[] = {0, 70, 154};
    uint8_t x = x_coord[i%3];
    lcdDrawNumber(x, y, i+1, LEADING0|LEFT, 2);
    lcdDrawChar(x+2*FW-2, y, ':');
#else
    coord_t y = MENU_HEADER_HEIGHT + 1 + (i/2)*FH;
    uint8_t x = (i & 1) ? LCD_W/2+FW : 0;
    drawStringWithIndex(x, y, "A", i+1);
    lcdDrawChar(lcdNextPos, y, ':');
#endif
    lcdDrawNumber(x+3*FW-1, y, anaIn(i), LEADING0|LEFT, 4);
    lcdDrawNumber(x+10*FW-1, y, (int16_t)calibratedAnalogs[CONVERT_MODE(i)]*25/256, RIGHT);
  }

#if (NUM_PWMSTICKS > 0) && !defined(SIMU)
  lcdDrawTextAlignedLeft(7*FH, STICKS_PWM_ENABLED() ? "Sticks: PWM" : "Sticks: ANA");
#endif
}
#endif // MENU_DIAG_ANAS_KEYS
