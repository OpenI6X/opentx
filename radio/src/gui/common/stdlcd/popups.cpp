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

uint8_t popupMenuOffsetType = MENU_OFFSET_INTERNAL;
void (*popupFunc)(event_t event) = NULL;

const char * popupMenuItems[POPUP_MENU_MAX_LINES];
uint8_t popupMenuSelectedItem = 0;
uint16_t popupMenuItemsCount = 0;
uint8_t popupMenuFlags = 0;
uint16_t popupMenuOffset = 0;
void (*popupMenuHandler)(const char * result);
const char * popupMenuTitle = nullptr;

const char * runPopupMenu(event_t event)
{
  const char * result = nullptr;

  uint8_t display_count = min<uint8_t>(popupMenuItemsCount, MENU_MAX_DISPLAY_LINES);
  uint8_t y = LCD_H / 2 - (popupMenuTitle ? 0 : 3) - (display_count * FH / 2);

  // white background
  lcdDrawFilledRect(MENU_X - 1, popupMenuTitle ? y - FH - 3 : y - 1, MENU_W + 2, display_count * (FH+1) + (popupMenuTitle ? FH + 6 : 4), SOLID, ERASE);
  
  // title
  if (popupMenuTitle) {
    lcdDrawText(MENU_X + 2, y - FH, popupMenuTitle, BOLD);
    lcdDrawRect(MENU_X, y - FH - 2, lcdLastRightPos - MENU_X + 2, FH + 3);
  }

  // border
  lcdDrawRect(MENU_X, y, MENU_W, display_count * (FH+1) + 2, SOLID, FORCE);

  // items
  for (uint32_t i=0; i<display_count; i++) {
    lcdDrawText(MENU_X+6, i*(FH+1) + y + 2, popupMenuItems[i+(popupMenuOffsetType == MENU_OFFSET_INTERNAL ? popupMenuOffset : 0)], 0);
    if (i == popupMenuSelectedItem) lcdDrawSolidFilledRect(MENU_X+1, i*(FH+1) + y + 1, MENU_W-2, 9);
  }

  if (popupMenuItemsCount > display_count) {
    drawVerticalScrollbar(MENU_X+MENU_W-1, y+1, MENU_MAX_DISPLAY_LINES * (FH+1), popupMenuOffset, popupMenuItemsCount, display_count);
  }

  switch (event) {
#if defined(ROTARY_ENCODER_NAVIGATION)
    CASE_EVT_ROTARY_LEFT
#endif
    case EVT_KEY_FIRST(KEY_UP):
    case EVT_KEY_REPT(KEY_UP):
      if (popupMenuSelectedItem > 0) {
        popupMenuSelectedItem--;
      }
#if defined(SDCARD)
      else if (popupMenuOffset > 0) {
        popupMenuOffset--;
        result = STR_UPDATE_LIST;
      }
#endif
      else {
        popupMenuSelectedItem = min<uint8_t>(display_count, MENU_MAX_DISPLAY_LINES) - 1;
#if defined(SDCARD)
        if (popupMenuItemsCount > MENU_MAX_DISPLAY_LINES) {
          popupMenuOffset = popupMenuItemsCount - display_count;
          result = STR_UPDATE_LIST;
        }
#endif
      }
      break;

#if defined(ROTARY_ENCODER_NAVIGATION)
    CASE_EVT_ROTARY_RIGHT
#endif
    case EVT_KEY_FIRST(KEY_DOWN):
    case EVT_KEY_REPT(KEY_DOWN):
      if (popupMenuSelectedItem < display_count - 1 && popupMenuOffset + popupMenuSelectedItem + 1 < popupMenuItemsCount) {
        popupMenuSelectedItem++;
      }
#if defined(SDCARD)
      else if (popupMenuItemsCount > popupMenuOffset + display_count) {
        popupMenuOffset++;
        result = STR_UPDATE_LIST;
      }
#endif
      else {
        popupMenuSelectedItem = 0;
#if defined(SDCARD)
        if (popupMenuOffset) {
          popupMenuOffset = 0;
          result = STR_UPDATE_LIST;
        }
#endif
      }
      break;

#if defined(CASE_EVT_ROTARY_BREAK)
    CASE_EVT_ROTARY_BREAK
#endif
    case EVT_KEY_BREAK(KEY_ENTER):
      result = popupMenuItems[popupMenuSelectedItem + (popupMenuOffsetType == MENU_OFFSET_INTERNAL ? popupMenuOffset : 0)];
      popupMenuItemsCount = 0;
      popupMenuSelectedItem = 0;
      popupMenuOffset = 0;
      popupMenuTitle = nullptr;
      break;

#if defined(CASE_EVT_ROTARY_LONG)
    CASE_EVT_ROTARY_LONG
      killEvents(event);
      // no break
#endif

    case EVT_KEY_BREAK(KEY_EXIT):
      result = STR_EXIT;
      popupMenuItemsCount = 0;
      popupMenuSelectedItem = 0;
      popupMenuFlags = 0;
      popupMenuOffset = 0;
      popupMenuTitle = nullptr;
      break;
  }

  return result;
}

