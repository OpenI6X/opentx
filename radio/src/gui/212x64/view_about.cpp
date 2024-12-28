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

const unsigned char about_bmp[]  = {
#include "about.lbm"
};

enum AboutScreens {
  ABOUT_OPENTX,
  ABOUT_HARDWARE,
  ABOUT_BERTRAND,
  ABOUT_ANDRE,
  ABOUT_MIKE,
  ABOUT_KJELL,
  ABOUT_MARTIN,
  ABOUT_ROMOLO,
  ABOUT_ROB,
  ABOUT_PARENTS,
  ABOUT_END,
  ABOUT_COUNT,
};

#define ABOUT_X      62
#define ABOUT_INDENT 6

void menuAboutView(event_t event)
{
  static uint8_t screenIndex;
  static uint8_t greyIndex;

  switch(event)
  {
    case EVT_ENTRY:
      screenIndex = 0;
      greyIndex = 0;
      break;
    case EVT_KEY_FIRST(KEY_DOWN):
      screenIndex < ABOUT_PARENTS ? screenIndex++ : screenIndex = ABOUT_OPENTX;
      greyIndex = 0;
      break;
    case EVT_KEY_FIRST(KEY_UP):
      screenIndex > ABOUT_OPENTX ? screenIndex-- : screenIndex = ABOUT_PARENTS;
      greyIndex = 0;
      break;
    case EVT_KEY_FIRST(KEY_EXIT):
      chainMenu(menuMainView);
      break;
  }

  lcdDrawBitmap(0, 0, about_bmp);
  lcdDrawText(64, 0, STR_ABOUTUS, DBLSIZE);
  lcdDrawSolidHorizontalLine(ABOUT_X, 18, 120);
  lcdDrawSolidHorizontalLine(ABOUT_X, 19, 130, GREY_DEFAULT);
  LcdFlags att = GREY(max(0, 15-greyIndex/2));

  uint8_t screenDuration = 150;

  if (++greyIndex == screenDuration) {
    greyIndex = 0;
    if (++screenIndex == ABOUT_COUNT) {
      chainMenu(menuMainView);
    }
  }
}
