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

uint8_t switchToMix(uint8_t source)
{
  div_t qr = div(source-1, 3);
  return qr.quot+MIXSRC_FIRST_SWITCH;
}

uint8_t expandableSection(coord_t y, const char* title, uint8_t value, uint8_t attr, event_t event)
{
  #define STR_CHAR_UP "\300"
  #define STR_CHAR_DOWN "\301"
  lcdDrawTextAlignedLeft(y, title);
  lcdDrawText(120, y, value ? STR_CHAR_UP : STR_CHAR_DOWN, attr);
  if (attr && (event == EVT_KEY_BREAK(KEY_ENTER))) {
    value = !value;
    s_editMode = 0;
  }
  return value;
}
