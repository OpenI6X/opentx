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

#define BIGSIZE       DBLSIZE
#if defined (PCBTARANIS) || defined(PCBI6X)
  #define LBOX_CENTERX  (LCD_W/4 + 14)
  #define RBOX_CENTERX  (3*LCD_W/4 - 13)
#else
  #define LBOX_CENTERX  (LCD_W/4 + 10)
  #define RBOX_CENTERX  (3*LCD_W/4 - 10)
#endif
#define MODELNAME_X   (2*FW-2)
#define MODELNAME_Y   (0)
#define PHASE_X       (6*FW-2)
#define PHASE_Y       (2*FH)
#define PHASE_FLAGS   0
#define VBATT_X       (6*FW-1)
#define VBATT_Y       (2*FH)
#define VBATTUNIT_X   (VBATT_X-1)
#define VBATTUNIT_Y   (3*FH)
#define REBOOT_X      (20*FW-3)
#define BAR_HEIGHT    (BOX_WIDTH-1l) // don't remove the l here to force 16bits maths on 9X
#define TRIM_LH_X     (LCD_W*1/4+2)
#define TRIM_LV_X     3
#define TRIM_RV_X     (LCD_W-4)
#define TRIM_RH_X     (LCD_W*3/4-2)
#define TRIM_LH_NEG   (TRIM_LH_X+1*FW)
#define TRIM_LH_POS   (TRIM_LH_X-4*FW)
#define TRIM_RH_NEG   (TRIM_RH_X+1*FW)
#define TRIM_RH_POS   (TRIM_RH_X-4*FW)
#if defined(TELEMETRY_FRSKY)
#define RSSSI_X       (30)
#define RSSSI_Y       (31)
#if defined(PCBI6X)
  #define RSSI_MAX      100
#else
  #define RSSI_MAX      105
#endif
#endif

#define TRIM_LEN      23

void drawExternalAntennaAndRSSI()
{
#if defined(INTERNAL_MODULE_PXX1) && defined(EXTERNAL_ANTENNA)
  if (isModuleXJT(INTERNAL_MODULE) && isExternalAntennaEnabled()) {
    lcdDrawText(VBATT_X-1, VBATT_Y+8, "E", TINSIZE);
  }
#endif

  if (TELEMETRY_RSSI() > 0) {
    auto warningRSSI = g_model.rssiAlarms.getWarningRssi();
    int8_t value = TELEMETRY_RSSI() - warningRSSI;
    uint8_t step = (RSSI_MAX - warningRSSI) / 4;
    for (uint8_t i = 1; i < 5; i++) {
      if (value > step * (i - 1)) {
        lcdDrawFilledRect(RSSSI_X + i * 4, RSSSI_Y - 2 * i + 1, 3, 2 * i - 1, SOLID, 0);
      }
    }
  }
}

void drawPotsBars()
{
  // Optimization by Mike Blandford
  for (uint8_t x=LCD_W/2 - (NUM_POTS+NUM_SLIDERS-1) * 5 / 2, i=NUM_STICKS; i<NUM_STICKS+NUM_POTS+NUM_SLIDERS; x+=5, i++) {
    if (IS_POT_SLIDER_AVAILABLE(i)) {
      uint8_t len = ((calibratedAnalogs[i]+RESX)*BAR_HEIGHT/(RESX*2))+1l;  // calculate once per loop
      V_BAR(x, LCD_H-8, len);
    }
  }
}

void doMainScreenGraphics()
{
  int16_t calibStickVert = calibratedAnalogs[CONVERT_MODE(1)];
  if (g_model.throttleReversed && CONVERT_MODE(1) == THR_STICK)
    calibStickVert = -calibStickVert;
  drawStick(LBOX_CENTERX, calibratedAnalogs[CONVERT_MODE(0)], calibStickVert);

  calibStickVert = calibratedAnalogs[CONVERT_MODE(2)];
  if (g_model.throttleReversed && CONVERT_MODE(2) == THR_STICK)
    calibStickVert = -calibStickVert;
  drawStick(RBOX_CENTERX, calibratedAnalogs[CONVERT_MODE(3)], calibStickVert);

  drawPotsBars();
}

void displayTrims(uint8_t phase)
{
  for (uint32_t i=0; i<4; i++) {
    static coord_t x[4] = {TRIM_LH_X, TRIM_LV_X, TRIM_RV_X, TRIM_RH_X};
    static uint8_t vert[4] = {0,1,1,0};
    coord_t xm, ym;
    uint8_t stickIndex = CONVERT_MODE(i);
    xm = x[stickIndex];
    uint8_t att = ROUND;
    int16_t val = getTrimValue(phase, i);

    if(getRawTrimValue(phase, i).mode == TRIM_MODE_NONE)
      continue;

    int16_t dir = val;
    bool exttrim = false;
    if (val < TRIM_MIN || val > TRIM_MAX) {
      exttrim = true;
    }
    if (val < -(TRIM_LEN+1)*4) {
      val = -(TRIM_LEN+1);
    }
    else if (val > (TRIM_LEN+1)*4) {
      val = TRIM_LEN+1;
    }
    else {
      val /= 4;
    }

    if (vert[i]) {
      ym = 31;
      lcdDrawSolidVerticalLine(xm, ym-TRIM_LEN, TRIM_LEN*2);
      if (i!=2 || !g_model.thrTrim) {
        lcdDrawSolidVerticalLine(xm-1, ym-1,  3);
        lcdDrawSolidVerticalLine(xm+1, ym-1,  3);
      }
      ym -= val;
      lcdDrawFilledRect(xm-3, ym-3, 7, 7, SOLID, att|ERASE);
      if (dir >= 0) {
        lcdDrawSolidHorizontalLine(xm-1, ym-1,  3);
      }
      if (dir <= 0) {
        lcdDrawSolidHorizontalLine(xm-1, ym+1,  3);
      }
      if (exttrim) {
        lcdDrawSolidHorizontalLine(xm-1, ym,  3);
      }
      if (g_model.displayTrims != DISPLAY_TRIMS_NEVER && dir != 0) {
        if (g_model.displayTrims == DISPLAY_TRIMS_ALWAYS || (trimsDisplayTimer > 0 && (trimsDisplayMask & (1<<i)))) {
          lcdDrawNumber(dir>0 ? 12 : 40, xm-2, -abs(dir/5), TINSIZE|VERTICAL);
        }
      }
    }
    else {
      ym = 60;
      lcdDrawSolidHorizontalLine(xm-TRIM_LEN, ym, TRIM_LEN*2);
      lcdDrawSolidHorizontalLine(xm-1, ym-1,  3);
      lcdDrawSolidHorizontalLine(xm-1, ym+1,  3);
      xm += val;
      lcdDrawFilledRect(xm-3, ym-3, 7, 7, SOLID, att|ERASE);
      if (dir >= 0) {
        lcdDrawSolidVerticalLine(xm+1, ym-1,  3);
      }
      if (dir <= 0) {
        lcdDrawSolidVerticalLine(xm-1, ym-1,  3);
      }
      if (exttrim) {
        lcdDrawSolidVerticalLine(xm, ym-1,  3);
      }
      if (g_model.displayTrims != DISPLAY_TRIMS_NEVER && dir != 0) {
        if (g_model.displayTrims == DISPLAY_TRIMS_ALWAYS || (trimsDisplayTimer > 0 && (trimsDisplayMask & (1<<i)))) {
          lcdDrawNumber((stickIndex==0 ? (dir>0 ? TRIM_LH_POS : TRIM_LH_NEG) : (dir>0 ? TRIM_RH_POS : TRIM_RH_NEG)), ym-2, -abs(dir/5), TINSIZE);
        }
      }
    }
    lcdDrawSquare(xm-3, ym-3, 7, att);
  }
}

void displayBattVoltage()
{
#if defined(BATTGRAPH)
  putsVBat(VBATT_X-8, VBATT_Y+1, RIGHT);
  lcdDrawSolidFilledRect(VBATT_X-25, VBATT_Y+9, 21, 5);
  lcdDrawSolidVerticalLine(VBATT_X-4, VBATT_Y+10, 3);
  uint8_t count = GET_TXBATT_BARS();
  for (uint32_t i=0; i<count; i+=2)
    lcdDrawSolidVerticalLine(VBATT_X-24+i, VBATT_Y+10, 3);
  if (!IS_TXBATT_WARNING() || BLINK_ON_PHASE)
    lcdDrawSolidFilledRect(VBATT_X-26, VBATT_Y, 24, 15);
#else
  LcdFlags att = (IS_TXBATT_WARNING() ? BLINK|INVERS : 0) | BIGSIZE;
  putsVBat(VBATT_X-1, VBATT_Y, att|NO_UNIT);
  lcdDrawChar(VBATT_X, VBATTUNIT_Y, 'V');
#endif
}

#define displayVoltageOrAlarm() displayBattVoltage()

#if defined(PCBX7)
  #define EVT_KEY_CONTEXT_MENU           EVT_KEY_LONG(KEY_ENTER)
  #define EVT_KEY_NEXT_VIEW              EVT_KEY_BREAK(KEY_PAGE)
  #define EVT_KEY_NEXT_PAGE              EVT_ROTARY_RIGHT
  #define EVT_KEY_PREVIOUS_PAGE          EVT_ROTARY_LEFT
  #define EVT_KEY_MODEL_MENU             EVT_KEY_BREAK(KEY_MENU)
  #define EVT_KEY_GENERAL_MENU           EVT_KEY_LONG(KEY_MENU)
  #define EVT_KEY_TELEMETRY              EVT_KEY_LONG(KEY_PAGE)
#elif defined(PCBXLITE)
  #define EVT_KEY_CONTEXT_MENU           EVT_KEY_LONG(KEY_ENTER)
  #define EVT_KEY_PREVIOUS_VIEW          EVT_KEY_BREAK(KEY_UP)
  #define EVT_KEY_NEXT_VIEW              EVT_KEY_BREAK(KEY_DOWN)
  #define EVT_KEY_NEXT_PAGE              EVT_KEY_BREAK(KEY_RIGHT)
  #define EVT_KEY_PREVIOUS_PAGE          EVT_KEY_BREAK(KEY_LEFT)
  #define EVT_KEY_MODEL_MENU             EVT_KEY_LONG(KEY_RIGHT)
  #define EVT_KEY_GENERAL_MENU           EVT_KEY_LONG(KEY_LEFT)
  #define EVT_KEY_TELEMETRY              EVT_KEY_LONG(KEY_DOWN)
  #define EVT_KEY_STATISTICS             EVT_KEY_LONG(KEY_UP)
#else
  #define EVT_KEY_CONTEXT_MENU           EVT_KEY_BREAK(KEY_MENU)
  #define EVT_KEY_PREVIOUS_VIEW          EVT_KEY_BREAK(KEY_UP)
  #define EVT_KEY_NEXT_VIEW              EVT_KEY_BREAK(KEY_DOWN)
  #define EVT_KEY_NEXT_PAGE              EVT_KEY_BREAK(KEY_RIGHT)
  #define EVT_KEY_PREVIOUS_PAGE          EVT_KEY_BREAK(KEY_LEFT)
  #if defined(PCBI6X)
    #define EVT_KEY_GENERAL_MENU           EVT_KEY_LONG(KEY_RIGHT)
    #define EVT_KEY_MODEL_MENU             EVT_KEY_LONG(KEY_MENU)
  #else
    #define EVT_KEY_GENERAL_MENU           EVT_KEY_LONG(KEY_LEFT)
    #define EVT_KEY_MODEL_MENU             EVT_KEY_LONG(KEY_RIGHT)
    #define EVT_KEY_LAST_MENU              EVT_KEY_LONG(KEY_MENU)
  #endif
  #define EVT_KEY_TELEMETRY              EVT_KEY_LONG(KEY_DOWN)
  #define EVT_KEY_STATISTICS             EVT_KEY_LONG(KEY_UP)
#endif

void onMainViewMenu(const char *result)
{
  if (result == STR_RESET_TIMER1) {
    timerReset(0);
  }
  else if (result == STR_RESET_TIMER2) {
    timerReset(1);
  }
#if TIMERS > 2
  else if (result == STR_RESET_TIMER3) {
    timerReset(2);
  }
#endif
#if defined(SDCARD)
  else if (result == STR_VIEW_NOTES) {
    pushModelNotes();
  }
#endif
  else if (result == STR_RESET_SUBMENU) {
    POPUP_MENU_ADD_ITEM(STR_RESET_FLIGHT);
    POPUP_MENU_ADD_ITEM(STR_RESET_TIMER1);
    POPUP_MENU_ADD_ITEM(STR_RESET_TIMER2);
    POPUP_MENU_ADD_ITEM(STR_RESET_TIMER3);
#if defined(TELEMETRY_FRSKY)
    POPUP_MENU_ADD_ITEM(STR_RESET_TELEMETRY);
#endif
  }
#if defined(TELEMETRY_FRSKY)
  else if (result == STR_RESET_TELEMETRY) {
    telemetryReset();
  }
#endif
  else if (result == STR_RESET_FLIGHT) {
    flightReset();
  }
  else if (result == STR_STATISTICS) {
    chainMenu(menuStatisticsView);
  }
  else if (result == STR_SAVEALLDATA) {
    watchdogSuspend(200); // 2s
    saveAllData();
  }
#if !defined(PCBI6X)
  else if (result == STR_ABOUT_US) {
    chainMenu(menuAboutView);
  }
#endif
}

void menuMainView(event_t event)
{
  uint8_t view = g_eeGeneral.view;
  uint8_t view_base = view & 0x0f;

  #define CHANNELS_PAGES ((MAX_OUTPUT_CHANNELS + 8 - 1) / 8)
  #define CHANNELS_SEGMENT_WIDTH (54 / CHANNELS_PAGES)

  switch (event) {
    case EVT_ENTRY:
      killEvents(KEY_EXIT);
      killEvents(KEY_UP);
      killEvents(KEY_DOWN);
      break;

    /* TODO if timer2 is OFF, it's possible to use this timer2 as in er9x...
    case EVT_KEY_BREAK(KEY_MENU):
      if (view_base == VIEW_TIMER2) {
        Timer2_running = !Timer2_running;
        AUDIO_KEY_PRESS();
      }
    break;
    */

    case EVT_KEY_NEXT_PAGE:
    case EVT_KEY_PREVIOUS_PAGE:
            if (view_base == VIEW_INPUTS)
              g_eeGeneral.view ^= ALTERNATE_VIEW;
            else
              g_eeGeneral.view = (g_eeGeneral.view + (CHANNELS_PAGES*ALTERNATE_VIEW) + ((event==EVT_KEY_PREVIOUS_PAGE) ? -ALTERNATE_VIEW : ALTERNATE_VIEW)) % (CHANNELS_PAGES*ALTERNATE_VIEW);
      break;

    case EVT_KEY_CONTEXT_MENU:
      killEvents(event);
#if defined(PCBI6X)
      if (view_base == VIEW_CHAN_MONITOR) break;
#else
      if (modelHasNotes()) {
        POPUP_MENU_ADD_ITEM(STR_VIEW_NOTES);
      }
#endif

#if defined(PCBI6X)
      POPUP_MENU_ADD_ITEM(STR_SAVEALLDATA);
#endif
      POPUP_MENU_ADD_ITEM(STR_RESET_SUBMENU);

      POPUP_MENU_ADD_ITEM(STR_STATISTICS);

#if !defined(PCBI6X)
      POPUP_MENU_ADD_ITEM(STR_ABOUT_US);
#endif
      POPUP_MENU_START(onMainViewMenu);
      break;

#if MENUS_LOCK != 2 /*no menus*/
#if defined(EVT_KEY_LAST_MENU)
    case EVT_KEY_LAST_MENU:
      pushMenu(lastPopMenu());
      killEvents(event);
      break;
#endif

    CASE_EVT_ROTARY_BREAK
    case EVT_KEY_MODEL_MENU:
      pushMenu(menuModelSelect);
      killEvents(event);
      break;

    CASE_EVT_ROTARY_LONG
    case EVT_KEY_GENERAL_MENU:
      pushMenu(menuTabGeneral[0]);
      killEvents(event);
      break;
#endif

#if defined(EVT_KEY_PREVIOUS_VIEW)
      // TODO try to split those 2 cases on 9X
    case EVT_KEY_PREVIOUS_VIEW:
    case EVT_KEY_NEXT_VIEW:
      // TODO try to split those 2 cases on 9X
      g_eeGeneral.view = (event == EVT_KEY_PREVIOUS_VIEW ? (view_base == VIEW_COUNT - 1 ? 0 : view_base + 1) : (view_base == 0 ? VIEW_COUNT - 1 : view_base - 1));
      storageDirty(EE_GENERAL);
      break;
#else
    case EVT_KEY_NEXT_VIEW:
      g_eeGeneral.view = (view_base == 0 ? VIEW_COUNT - 1 : view_base - 1);
      storageDirty(EE_GENERAL);
      break;
#endif

#if defined(EVT_KEY_STATISTICS)
    case EVT_KEY_STATISTICS:
      chainMenu(menuStatisticsView);
      killEvents(event);
      break;
#endif

    case EVT_KEY_TELEMETRY:
#if defined(TELEMETRY_FRSKY)
      chainMenu(menuViewTelemetry);
#else
      chainMenu(menuStatisticsDebug);
#endif
      killEvents(event);
      break;

    case EVT_KEY_FIRST(KEY_EXIT):
#if defined(GVARS) && !defined(PCBSTD)
      if (gvarDisplayTimer > 0) {
        gvarDisplayTimer = 0;
      }
#endif
      break;
  }

  switch (view_base) {
    case VIEW_CHAN_MONITOR:
      menuChannelsViewCommon(event);
      break;
    case VIEW_OUTPUTS_VALUES:
    case VIEW_OUTPUTS_BARS:
      // scroll bar
      lcdDrawHorizontalLine(38, 34, 54, DOTTED);
      lcdDrawSolidHorizontalLine(38 + (g_eeGeneral.view / ALTERNATE_VIEW) * CHANNELS_SEGMENT_WIDTH, 34, CHANNELS_SEGMENT_WIDTH, SOLID);
      for (uint32_t i=0; i<8; i++) {
        uint8_t x0, y0;
        uint8_t chan = 8 * (g_eeGeneral.view / ALTERNATE_VIEW) + i;
        int16_t val = channelOutputs[chan];

        if (view_base == VIEW_OUTPUTS_VALUES) {
          x0 = (i % 4 * 9 + 3) * FW / 2;
          y0 = i / 4 * FH + 40;
          drawChannelValue(x0 + 4 * FW, y0, chan, val, RIGHT);
        }
        else {
          constexpr coord_t WBAR2 =  (50/2);
          x0 = i<4 ? LCD_W/4+2 : LCD_W*3/4-2;
          y0 = 38+(i%4)*5;

          const uint16_t lim = (g_model.extendedLimits ? (512 * (long)LIMIT_EXT_PERCENT / 100) : 512) * 2;
          int8_t len = (abs(val) * WBAR2 + lim/2) / lim;

          if (len>WBAR2)
            len = WBAR2; // prevent bars from going over the end - comment for debugging
          lcdDrawHorizontalLine(x0-WBAR2, y0, WBAR2*2+1, DOTTED);
          lcdDrawSolidVerticalLine(x0, y0-2,5 );
          if (val > 0)
            x0 += 1;
          else
            x0 -= len;
          lcdDrawSolidHorizontalLine(x0, y0+1, len);
          lcdDrawSolidHorizontalLine(x0, y0-1, len);
        }
      }
      break;

    case VIEW_TIMER2:
      drawTimerWithMode(87, 5 * FH, 1, RIGHT | DBLSIZE);
      break;

    case VIEW_INPUTS:
      if (view == VIEW_INPUTS) {
        // Sticks + Pots
        doMainScreenGraphics();

        // Switches
#if defined(PCBX9LITES)
        static const uint8_t x[NUM_SWITCHES-2] = {2*FW-2, 2*FW-2, 17*FW+1, 2*FW-2, 17*FW+1};
        static const uint8_t y[NUM_SWITCHES-2] = {4*FH+1, 5*FH+1, 5*FH+1, 6*FH+1, 6*FH+1};
        for (int i=0; i<NUM_SWITCHES - 2; ++i) {
          if (SWITCH_EXISTS(i)) {
            getvalue_t val = getValue(MIXSRC_FIRST_SWITCH + i);
            getvalue_t sw = ((val < 0) ? 3 * i + 1 : ((val == 0) ? 3 * i + 2 : 3 * i + 3));
            drawSwitch(x[i], y[i], sw, 0, false);
          }
        }
        drawSmallSwitch(29, 5*FH+1, 4, SW_SF);
        drawSmallSwitch(16*FW+1, 5*FH+1, 4, SW_SG);
#elif defined(PCBX9LITE)
        static const uint8_t x[NUM_SWITCHES] = {2 * FW - 2, 2 * FW - 2, 16 * FW + 1, 2 * FW - 2, 16 * FW + 1};
        static const uint8_t y[NUM_SWITCHES] = {4 * FH + 1, 5 * FH + 1, 5 * FH + 1, 6 * FH + 1, 6 * FH + 1};
        for (int i = 0; i < NUM_SWITCHES; ++i) {
          if (SWITCH_EXISTS(i)) {
            getvalue_t val = getValue(MIXSRC_FIRST_SWITCH + i);
            getvalue_t sw = ((val < 0) ? 3 * i + 1 : ((val == 0) ? 3 * i + 2 : 3 * i + 3));
            drawSwitch(x[i], y[i], sw, 0, false);
          }
        }
#elif defined(PCBXLITES)
        static const uint8_t x[NUM_SWITCHES] = {2*FW-2, 16*FW+1, 2*FW-2, 16*FW+1, 2*FW-2, 16*FW+1};
        static const uint8_t y[NUM_SWITCHES] = {4*FH+1, 4*FH+1, 6*FH+1, 6*FH+1, 5*FH+1, 5*FH+1};
        for (int i=0; i<NUM_SWITCHES; ++i) {
          if (SWITCH_EXISTS(i)) {
            getvalue_t val = getValue(MIXSRC_FIRST_SWITCH + i);
            getvalue_t sw = ((val < 0) ? 3 * i + 1 : ((val == 0) ? 3 * i + 2 : 3 * i + 3));
            drawSwitch(x[i], y[i], sw, 0, false);
          }
        }
#elif defined(PCBTARANIS) || defined(PCBI6X)
        uint8_t switches = min((int)NUM_SWITCHES, 6);
        for (int i = 0; i < switches; ++i) {
          if (SWITCH_EXISTS(i)) {
            uint8_t x = 2 * FW - 2, y = 4 * FH + i * FH + 1;
            if (i >= switches / 2) {
              x = 16 * FW + 1;
              y -= (switches / 2) * FH;
            }
            getvalue_t val = getValue(MIXSRC_FIRST_SWITCH + i);
            getvalue_t sw = ((val < 0) ? 3 * i + 1 : ((val == 0) ? 3 * i + 2 : 3 * i + 3));
            drawSwitch(x, y, sw, 0, false);
          }
        }
#else
        // The ID0 3-POS switch is merged with the TRN switch
        for (uint32_t i=SWSRC_THR; i<=SWSRC_TRN; i++) {
          int8_t sw = (i == SWSRC_TRN ? (switchState(SW_ID0) ? SWSRC_ID0 : (switchState(SW_ID1) ? SWSRC_ID1 : SWSRC_ID2)) : i);
          uint8_t x = 2*FW-2, y = i*FH+1;
          if (i >= SWSRC_AIL) {
            x = 17*FW-1;
            y -= 3*FH;
          }
          drawSwitch(x, y, sw, getSwitch(i) ? INVERS : 0, false);
        }
#endif
      }
      else {
        // Logical Switches
        uint8_t index = 0;
        uint8_t y = LCD_H - 20;
        for (uint8_t line = 0; line < 2; line++) {
          for (uint8_t column = 0; column < MAX_LOGICAL_SWITCHES / 2; column++) {
            int8_t len = getSwitch(SWSRC_SW1 + index) ? 10 : 1;
            uint8_t x = (16 + 3 * column);
            lcdDrawSolidVerticalLine(x - 1, y - len, len);
            lcdDrawSolidVerticalLine(x, y - len, len);
            index++;
          }
          y += 12;
        }
      }
      break;
  }

  if (view_base != VIEW_CHAN_MONITOR) {
    // Flight Mode Name
    uint8_t mode = mixerCurrentFlightMode;
    lcdDrawSizedText(PHASE_X, PHASE_Y, g_model.flightModeData[mode].name, sizeof(g_model.flightModeData[mode].name), ZCHAR | PHASE_FLAGS);

    // Model Name
    putsModelName(MODELNAME_X, MODELNAME_Y, g_model.header.name, g_eeGeneral.currModel, BIGSIZE);

    // Main Voltage (or alarm if any)
    displayVoltageOrAlarm();

    // Timer 1
    drawTimerWithMode(125, 2 * FH, 0, RIGHT | DBLSIZE);

    // Trims sliders
    displayTrims(mode);

    // RSSI gauge / external antenna
    drawExternalAntennaAndRSSI();

    // And ! in case of unexpected shutdown
    if (isAsteriskDisplayed()) {
      lcdDrawChar(REBOOT_X, 0 * FH, '!', INVERS | BLINK);
    }

#if defined(PCBI6X)
    // Add square in case of pending or ongoing eeprom write
    if (storageDirtyMsk || eepromIsWriting()) {
      lcdDrawRect(REBOOT_X + 3, 0 * FH, 4, 4);
    }
#endif // PCBI6X
  }

#if defined(GVARS)
  if (gvarDisplayTimer > 0) {
    gvarDisplayTimer--;
    warningText = STR_GLOBAL_VAR;
    showMessageBox(warningText); // drawMessageBox(warningText);
    lcdDrawSizedText(16, 5 * FH, g_model.gvars[gvarLastChanged].name, LEN_GVAR_NAME, ZCHAR);
    lcdDrawText(16 + 6 * FW, 5 * FH, "[", BOLD);
    drawGVarValue(lcdLastRightPos, 5 * FH, gvarLastChanged, GVAR_VALUE(gvarLastChanged, getGVarFlightMode(mixerCurrentFlightMode, gvarLastChanged)),
                  LEFT | BOLD);
    if (g_model.gvars[gvarLastChanged].unit) {
      lcdDrawText(lcdLastRightPos, 5 * FH, "%", BOLD);
    }
    lcdDrawText(lcdLastRightPos, 5 * FH, "]", BOLD);
    warningText = nullptr;
  }
#endif

#if defined(DSM2)
  if (moduleState[0].mode == MODULE_MODE_BIND) {
    // Issue 98
    lcdDrawText(15 * FW, 0, "BIND", 0);
  }
#endif
}

#undef EVT_KEY_CONTEXT_MENU
#undef EVT_KEY_PREVIOUS_VIEW
#undef EVT_KEY_NEXT_VIEW
#undef EVT_KEY_NEXT_PAGE
#undef EVT_KEY_PREVIOUS_PAGE
#undef EVT_KEY_MODEL_MENU
#undef EVT_KEY_GENERAL_MENU
#undef EVT_KEY_LAST_MENU
#undef EVT_KEY_TELEMETRY
#undef EVT_KEY_STATISTICS
