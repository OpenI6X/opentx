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

const char * warningText = NULL;
const char * warningInfoText;
uint8_t         warningInfoLength;
uint8_t         warningType;
uint8_t         warningResult = 0;

uint8_t         warningInfoFlags = ZCHAR;

void drawMessageBox()
{
  lcdDrawFilledRect(MESSAGEBOX_X - 1, MESSAGEBOX_Y - 1, MESSAGEBOX_W + 2, 48 + 2, SOLID, ERASE);
  lcdDrawRect(MESSAGEBOX_X, MESSAGEBOX_Y, MESSAGEBOX_W, 48);
  lcdDrawSizedText(WARNING_LINE_X, WARNING_LINE_Y, warningText, WARNING_LINE_LEN);
  // could be a place for a warningInfoText
}

void showMessageBox(const char * str)
{
  warningText = str;
  drawMessageBox();
  warningText = NULL;
  lcdRefresh();
}

const unsigned char ASTERISK_BITMAP[]  = {
#include "asterisk.lbm"
};

void drawAlertBox(const char * title, const char * text, const char * action)
{
  lcdClear();

#if defined(DEBUG) || (defined(PCBI6X_ELRS) && defined(TRANSLATIONS_PT)) // saves ~140B
  lcdDrawRect(2, 2, 32 - 4, 32 - 4);
  lcdDrawChar(11, 6, 'x', DBLSIZE);
#else
  lcdDraw1bitBitmap(2, 0, ASTERISK_BITMAP, 0, 0);
#endif

#define MESSAGE_LCD_OFFSET   6*FW

#if defined(TRANSLATIONS_FR) || defined(TRANSLATIONS_IT) || defined(TRANSLATIONS_CZ)
  lcdDrawText(MESSAGE_LCD_OFFSET, 0, STR_WARNING, DBLSIZE);
  lcdDrawText(MESSAGE_LCD_OFFSET, 2*FH, title, DBLSIZE);
#else
  lcdDrawText(MESSAGE_LCD_OFFSET, 0, title, DBLSIZE);
  lcdDrawText(MESSAGE_LCD_OFFSET, 2*FH, STR_WARNING, DBLSIZE);
#endif
  
  lcdDrawSolidFilledRect(0, 0, LCD_W, 32);
  if (text) {
    lcdDrawTextAlignedLeft(5*FH, text);
  }
  if (action) {
    lcdDrawTextAlignedLeft(7*FH, action);
  }
  
#undef MESSAGE_LCD_OFFSET
}

void showAlertBox(const char * title, const char * text, const char * action , uint8_t sound)
{
  drawAlertBox(title, text, action);
  
  AUDIO_ERROR_MESSAGE(sound);
  
  lcdRefresh();
  lcdSetContrast();
  waitKeysReleased();
  resetBacklightTimeout();
  checkBacklight();
}

void runPopupWarning(event_t event)
{
  warningResult = false;
  drawMessageBox();
  if (warningInfoText) {
    lcdDrawSizedText(WARNING_LINE_X, WARNING_LINE_Y+FH, warningInfoText, warningInfoLength, warningInfoFlags);
  }
  lcdDrawText(WARNING_LINE_X, WARNING_LINE_Y+4*FH+2, warningType == WARNING_TYPE_ASTERISK ? STR_EXIT : STR_POPUPS_ENTER_EXIT);
  
  switch (event) {
    case EVT_KEY_BREAK(KEY_ENTER):
      if (warningType == WARNING_TYPE_ASTERISK)
        break;
      warningResult = true;
      // no break
    case EVT_KEY_BREAK(KEY_EXIT):
      warningText = NULL;
      warningType = WARNING_TYPE_ASTERISK;
      break;
  }
}
