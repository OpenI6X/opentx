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

#if defined(RADIO_TOOLS)
void addRadioTool(uint8_t index, const char * label, void (* tool)(event_t event), event_t event)
{
  int8_t sub = menuVerticalPosition - HEADER_LINE;
  LcdFlags attr = (sub == index ? INVERS : 0);
  coord_t y = MENU_HEADER_HEIGHT + 1 + index * FH;
  lcdDrawText(0, y, label, attr);
  if (attr && s_editMode > 0) {
    s_editMode = 0;
    pushMenu(tool);
  }
}

void menuRadioTools(event_t event)
{
  uint8_t index = 0;

#if defined(PCBI6X_ELRS)
  extern void elrsRun(event_t event); 
  addRadioTool(index++, "ExpressLRS", elrsRun, event);
#endif

#if defined(PCBI6X_HELLO)
  extern void hello_run(event_t event);
  addRadioTool(index++, "Hello World", hello_run, event);
#endif

#if defined(PCBI6X_INAV)
  extern void inavRun(event_t event);
  addRadioTool(index++, "INAV Lite", inavRun, event);
#endif

  SIMPLE_MENU("TOOLS", menuTabGeneral, MENU_RADIO_TOOLS, HEADER_LINE + index);
}
#endif // RADIO_TOOLS
