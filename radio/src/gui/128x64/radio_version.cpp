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

#if defined(PXX2)
void menuRadioModulesVersion(event_t event)
{
  if (event == EVT_ENTRY) {
    moduleSettings[INTERNAL_MODULE].mode = MODULE_MODE_GET_HARDWARE_INFO;
    reusableBuffer.hardware.modules[INTERNAL_MODULE].step = -1;
    reusableBuffer.hardware.modules[INTERNAL_MODULE].timeout = 0;
  }

  SIMPLE_SUBMENU("MODULES / RX VERSION", 0);
}
#endif

enum MenuRadioVersionItems
{
  ITEM_RADIO_VERSION_FIRST = HEADER_LINE - 1,
#if defined(PXX2)
  ITEM_RADIO_MODULES_VERSION,
#endif
  ITEM_RADIO_VERSION_COUNT
};

void menuRadioVersion(event_t event)
{
  SIMPLE_MENU(STR_MENUVERSION, menuTabGeneral, MENU_RADIO_VERSION, ITEM_RADIO_VERSION_COUNT);

  coord_t y = MENU_HEADER_HEIGHT + 2;
  lcdDrawText(FW, y, vers_stamp, SMLSIZE);
  y += 5 * (FH - 1);

}