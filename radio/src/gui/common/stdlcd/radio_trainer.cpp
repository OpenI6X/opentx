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

#if LCD_W >= 212
  #define TRAINER_CALIB_COLUMN_WIDTH 12
#else
  #define TRAINER_CALIB_COLUMN_WIDTH 8
#endif

void menuRadioTrainer(event_t event)
{
  uint8_t y;
  bool slave = SLAVE_MODE();

  MENU(STR_MENUTRAINER, menuTabGeneral, MENU_RADIO_TRAINER, (slave ? HEADER_LINE : HEADER_LINE+6), { HEADER_LINE_COLUMNS NAVIGATION_LINE_BY_LINE|2, NAVIGATION_LINE_BY_LINE|2, NAVIGATION_LINE_BY_LINE|2, NAVIGATION_LINE_BY_LINE|2, 0, 0 });

  if (slave) {
    lcdDrawText(LCD_W/2, 4*FH, STR_SLAVE, CENTERED);
    return;
  }

  LcdFlags attr;
  LcdFlags blink = ((s_editMode>0) ? BLINK|INVERS : INVERS);

  lcdDrawText(5*FW, MENU_HEADER_HEIGHT+1, STR_MODE);
  lcdDrawText(11*FW, MENU_HEADER_HEIGHT+1, "%", RIGHT);
  lcdDrawText(12*FW, MENU_HEADER_HEIGHT+1, STR_SOURCE);

  y = MENU_HEADER_HEIGHT + 1 + FH;

  for (uint32_t i=HEADER_LINE; i<HEADER_LINE+NUM_STICKS; i++) {
    uint8_t chan = channelOrder(i+1-HEADER_LINE);
    TrainerMix * td = &g_eeGeneral.trainer.mix[chan-1];

    drawSource(0, y, MIXSRC_Rud-1+chan, (menuVerticalPosition==i && CURSOR_ON_LINE()) ? INVERS : 0);

    for (uint8_t j=0; j<3; j++) {

      attr = ((menuVerticalPosition==i && menuHorizontalPosition==j) ? blink : 0);

      switch (j) {
        case 0:
          lcdDrawTextAtIndex(5*FW, y, STR_TRNMODE, td->mode, attr);
          if (attr&BLINK) CHECK_INCDEC_GENVAR(event, td->mode, 0, 2);
          break;

        case 1:
          lcdDrawNumber(11*FW, y, td->studWeight, attr|RIGHT);
          if (attr&BLINK) CHECK_INCDEC_GENVAR(event, td->studWeight, -125, 125);
          break;

        case 2:
          lcdDrawTextAtIndex(12*FW, y, STR_TRNCHN, td->srcChn, attr);
          if (attr&BLINK) CHECK_INCDEC_GENVAR(event, td->srcChn, 0, 3);
          break;
      }
    }
    y += FH;
  }

  attr = (menuVerticalPosition==HEADER_LINE+4) ? blink : 0;
  lcdDrawTextAlignedLeft(MENU_HEADER_HEIGHT+1+5*FH, STR_MULTIPLIER);
  lcdDrawNumber(LEN_MULTIPLIER*FW+3*FW, MENU_HEADER_HEIGHT+1+5*FH, g_eeGeneral.PPM_Multiplier+10, attr|PREC1|RIGHT);
  if (attr) CHECK_INCDEC_GENVAR(event, g_eeGeneral.PPM_Multiplier, -10, 40);

  attr = (menuVerticalPosition==HEADER_LINE+5) ? INVERS : 0;
  if (attr)
    s_editMode = 0;
  lcdDrawText(0*FW, MENU_HEADER_HEIGHT+1+6*FH, STR_CAL, attr);
  for (uint32_t i=0; i<4; i++) {
    uint8_t x = (i*TRAINER_CALIB_COLUMN_WIDTH + 16) * FW/2;
    if (g_eeGeneral.ppmunit == PPM_PERCENT_PREC1) {
      lcdDrawNumber(x, MENU_HEADER_HEIGHT+1+6*FH, (trainerInput[i]-g_eeGeneral.trainer.calib[i])*2, PREC1|RIGHT);
    } else {
      lcdDrawNumber(x, MENU_HEADER_HEIGHT+1+6*FH, (trainerInput[i]-g_eeGeneral.trainer.calib[i])/5, RIGHT);
    }
  }

  if (attr) {
    s_editMode = 0;
    if (event==EVT_KEY_LONG(KEY_ENTER)){
      memcpy(g_eeGeneral.trainer.calib, trainerInput, sizeof(g_eeGeneral.trainer.calib));
      storageDirty(EE_GENERAL);
      AUDIO_WARNING1();
    }
  }
}
