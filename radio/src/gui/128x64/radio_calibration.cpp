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

#define XPOT_DELTA 10
#define XPOT_DELAY 10 /* cycles */

enum CalibrationState {
  CALIB_START = 0,
  CALIB_SET_MIDPOINT,
  CALIB_MOVE_STICKS,
  CALIB_STORE,
  CALIB_FINISHED
};

void menuCommonCalib(event_t event)
{
  for (uint32_t i=0; i<NUM_STICKS+NUM_POTS+NUM_SLIDERS; i++) { // get low and high vals for sticks and trims
    int16_t vt = anaIn(i);
    reusableBuffer.calib.loVals[i] = min(vt, reusableBuffer.calib.loVals[i]);
    reusableBuffer.calib.hiVals[i] = max(vt, reusableBuffer.calib.hiVals[i]);
    if (i >= POT1 && i <= POT_LAST) {
      if (IS_POT_WITHOUT_DETENT(i)) {
        reusableBuffer.calib.midVals[i] = (reusableBuffer.calib.hiVals[i] + reusableBuffer.calib.loVals[i]) / 2;
      }
#if defined(PCBTARANIS)
      uint8_t idx = i - POT1;
      int count = reusableBuffer.calib.xpotsCalib[idx].stepsCount;
      if (IS_POT_MULTIPOS(i) && count <= XPOTS_MULTIPOS_COUNT) {
        // use raw analog value for multipos calibraton, anaIn() already has multipos decoded value
        vt = getAnalogValue(i) >> 1;
        if (reusableBuffer.calib.xpotsCalib[idx].lastCount == 0 || vt < reusableBuffer.calib.xpotsCalib[idx].lastPosition - XPOT_DELTA || vt > reusableBuffer.calib.xpotsCalib[idx].lastPosition + XPOT_DELTA) {
          reusableBuffer.calib.xpotsCalib[idx].lastPosition = vt;
          reusableBuffer.calib.xpotsCalib[idx].lastCount = 1;
        }
        else {
          if (reusableBuffer.calib.xpotsCalib[idx].lastCount < 255) reusableBuffer.calib.xpotsCalib[idx].lastCount++;
        }
        if (reusableBuffer.calib.xpotsCalib[idx].lastCount == XPOT_DELAY) {
          int16_t position = reusableBuffer.calib.xpotsCalib[idx].lastPosition;
          bool found = false;
          for (int j=0; j<count; j++) {
            int16_t step = reusableBuffer.calib.xpotsCalib[idx].steps[j];
            if (position >= step-XPOT_DELTA && position <= step+XPOT_DELTA) {
              found = true;
              break;
            }
          }
          if (!found) {
            if (count < XPOTS_MULTIPOS_COUNT) {
              reusableBuffer.calib.xpotsCalib[idx].steps[count] = position;
            }
            reusableBuffer.calib.xpotsCalib[idx].stepsCount += 1;
          }
        }
      }
#endif
    }
  }

  menuCalibrationState = reusableBuffer.calib.state; // make sure we don't scroll while calibrating

  switch (event) {
    case EVT_ENTRY:
    case EVT_KEY_BREAK(KEY_EXIT):
      reusableBuffer.calib.state = CALIB_START;
      break;

    case EVT_KEY_BREAK(KEY_ENTER):
      reusableBuffer.calib.state++;
      break;
  }

  switch (reusableBuffer.calib.state) {
    case CALIB_START:
      // START CALIBRATION
      if (!READ_ONLY()) {
        lcdDrawTextAlignedLeft(MENU_HEADER_HEIGHT+2*FH, STR_MENUTOSTART);
      }
      break;

    case CALIB_SET_MIDPOINT:
      // SET MIDPOINT
      lcdDrawText(0*FW, MENU_HEADER_HEIGHT+FH, STR_SETMIDPOINT, INVERS);
      lcdDrawTextAlignedLeft(MENU_HEADER_HEIGHT+2*FH, STR_MENUWHENDONE);
      for (uint32_t i=0; i<NUM_STICKS+NUM_POTS+NUM_SLIDERS; i++) {
        reusableBuffer.calib.loVals[i] = 15000;
        reusableBuffer.calib.hiVals[i] = -15000;
#if defined(PCBTARANIS)
        reusableBuffer.calib.midVals[i] = getAnalogValue(i) >> 1;
        if (i<NUM_XPOTS) {
          reusableBuffer.calib.xpotsCalib[i].stepsCount = 0;
          reusableBuffer.calib.xpotsCalib[i].lastCount = 0;
        }
#else
        reusableBuffer.calib.midVals[i] = anaIn(i);
#endif
      }
      break;

    case CALIB_MOVE_STICKS:
      // MOVE STICKS/POTS
      lcdDrawText(0*FW, MENU_HEADER_HEIGHT+FH, STR_MOVESTICKSPOTS, INVERS);
      lcdDrawTextAlignedLeft(MENU_HEADER_HEIGHT+2*FH, STR_MENUWHENDONE);
      for (uint32_t i=0; i<NUM_STICKS+NUM_POTS+NUM_SLIDERS; i++) {
        if (abs(reusableBuffer.calib.loVals[i]-reusableBuffer.calib.hiVals[i]) > 50) {
          g_eeGeneral.calib[i].mid = reusableBuffer.calib.midVals[i];
          int16_t v = reusableBuffer.calib.midVals[i] - reusableBuffer.calib.loVals[i];
          g_eeGeneral.calib[i].spanNeg = v - v/STICK_TOLERANCE;
          v = reusableBuffer.calib.hiVals[i] - reusableBuffer.calib.midVals[i];
          g_eeGeneral.calib[i].spanPos = v - v/STICK_TOLERANCE;
        }
      }
      break;

    case CALIB_STORE:
#if defined(PCBTARANIS)
      for (uint32_t i=POT1; i<=POT_LAST; i++) {
        int idx = i - POT1;
        int count = reusableBuffer.calib.xpotsCalib[idx].stepsCount;
        if (IS_POT_MULTIPOS(i)) {
          if (count > 1 && count <= XPOTS_MULTIPOS_COUNT) {
            for (int j=0; j<count; j++) {
              for (int k=j+1; k<count; k++) {
                if (reusableBuffer.calib.xpotsCalib[idx].steps[k] < reusableBuffer.calib.xpotsCalib[idx].steps[j]) {
                  SWAP(reusableBuffer.calib.xpotsCalib[idx].steps[j], reusableBuffer.calib.xpotsCalib[idx].steps[k]);
                }
              }
            }
            StepsCalibData * calib = (StepsCalibData *) &g_eeGeneral.calib[i];
            calib->count = count - 1;
            for (int j=0; j<calib->count; j++) {
              calib->steps[j] = (reusableBuffer.calib.xpotsCalib[idx].steps[j+1] + reusableBuffer.calib.xpotsCalib[idx].steps[j]) >> 5;
            }
          }
          else {
            g_eeGeneral.potsConfig &= ~(0x03<<(2*idx));
          }
        }
      }
#endif
      g_eeGeneral.chkSum = evalChkSum();
      storageDirty(EE_GENERAL);
      reusableBuffer.calib.state = CALIB_FINISHED;
      break;

    default:
      reusableBuffer.calib.state = CALIB_START;
      break;
  }

  doMainScreenGraphics();
}

void menuRadioCalibration(event_t event)
{
  check_submenu_simple(event, 0);
  TITLE(STR_MENUCALIBRATION);
  menuCommonCalib(READ_ONLY() ? 0 : event);
  if (menuEvent) {
    menuCalibrationState = CALIB_START;
  }
}

void menuFirstCalib(event_t event)
{
  if (event == EVT_KEY_BREAK(KEY_EXIT) || reusableBuffer.calib.state == CALIB_FINISHED) {
    menuCalibrationState = CALIB_START;
    chainMenu(menuMainView);
  }
  else {
    lcdDrawText(LCD_W / 2, 0, STR_MENUCALIBRATION, CENTERED);
    lcdInvertLine(0);
    menuCommonCalib(event);
  }
}
