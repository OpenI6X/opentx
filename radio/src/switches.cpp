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

#define CS_LAST_VALUE_INIT -32768

#if defined(PCBHORUS)
  #define SWITCH_WARNING_LIST_X        WARNING_LINE_X
  #define SWITCH_WARNING_LIST_Y        WARNING_LINE_Y+3*FH
  #define SWITCH_WARNING_LIST_INTERVAL 35
#elif LCD_W >= 212
  #define SWITCH_WARNING_LIST_X        60
  #define SWITCH_WARNING_LIST_Y        4*FH+3
#else
  #define SWITCH_WARNING_LIST_X        4
  #define SWITCH_WARNING_LIST_Y        4*FH+4
#endif


enum LogicalSwitchContextState {
  SWITCH_START,
  SWITCH_DELAY,
  SWITCH_ENABLE
};

PACK(struct LogicalSwitchContext {
  uint8_t state:1;
  uint8_t timerState:2;
  uint8_t spare:5;
  uint8_t timer;
  int16_t lastValue;
});

PACK(struct LogicalSwitchesFlightModeContext {
  LogicalSwitchContext lsw[MAX_LOGICAL_SWITCHES];
});
LogicalSwitchesFlightModeContext lswFm[MAX_FLIGHT_MODES];

#define LS_LAST_VALUE(fm, idx) lswFm[fm].lsw[idx].lastValue


#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBI6X)
tmr10ms_t switchesMidposStart[NUM_SWITCHES];
uint64_t  switchesPos = 0;
tmr10ms_t potsLastposStart[NUM_XPOTS];
uint8_t   potsPos[NUM_XPOTS];

#define SWITCH_POSITION(sw)  (switchesPos & ((MASK_CFN_TYPE)1<<(sw)))
#define POT_POSITION(sw)     ((potsPos[(sw)/XPOTS_MULTIPOS_COUNT] & 0x0f) == ((sw) % XPOTS_MULTIPOS_COUNT))

div_t switchInfo(int switchPosition)
{
  return div(switchPosition-SWSRC_FIRST_SWITCH, 3);
}

uint64_t check2PosSwitchPosition(uint8_t sw)
{
  uint32_t index = (switchState(sw) ? sw : sw + 2);
  uint64_t result = ((uint64_t)1 << index);

  if (!(switchesPos & result)) {
    PLAY_SWITCH_MOVED(index);
  }

  return result;
}

uint64_t check3PosSwitchPosition(uint8_t idx, uint8_t sw, bool startup)
{
  uint64_t result;
  uint32_t index;

  if (switchState(sw)) {
    index = sw;
    result = ((MASK_CFN_TYPE)1 << index);
    switchesMidposStart[idx] = 0;
  }
  else if (switchState(sw+2)) {
    index = sw + 2;
    result = ((MASK_CFN_TYPE)1 << index);
    switchesMidposStart[idx] = 0;
  }
  else {
    index = sw + 1;
    if (startup || SWITCH_POSITION(index) || g_eeGeneral.switchesDelay==SWITCHES_DELAY_NONE || (switchesMidposStart[idx] && (tmr10ms_t)(get_tmr10ms() - switchesMidposStart[idx]) > SWITCHES_DELAY())) {
      result = ((MASK_CFN_TYPE)1 << index);
      switchesMidposStart[idx] = 0;
    }
    else {
      result = (switchesPos & ((MASK_CFN_TYPE)0x7 << sw));
      if (!switchesMidposStart[idx]) {
        switchesMidposStart[idx] = get_tmr10ms();
      }
    }
  }

  if (!(switchesPos & result)) {
    PLAY_SWITCH_MOVED(index);
  }

  return result;
}

#define CHECK_2POS(sw)       newPos |= check2PosSwitchPosition(sw ## 0)
#define CHECK_3POS(idx, sw)  newPos |= check3PosSwitchPosition(idx, sw ## 0, startup)

void getSwitchesPosition(bool startup)
{
  uint64_t newPos = 0;

  CHECK_3POS(0, SW_SA);
  CHECK_3POS(1, SW_SB);
  CHECK_3POS(2, SW_SC);
  CHECK_3POS(3, SW_SD);

  switchesPos = newPos;

  for (int i=0; i<NUM_XPOTS; i++) {
    if (IS_POT_MULTIPOS(POT1+i)) {
      StepsCalibData * calib = (StepsCalibData *) &g_eeGeneral.calib[POT1+i];
      if (IS_MULTIPOS_CALIBRATED(calib)) {
        uint8_t pos = anaIn(POT1+i) / (2*RESX/calib->count);
        uint8_t previousPos = potsPos[i] >> 4;
        uint8_t previousStoredPos = potsPos[i] & 0x0F;
        if (startup) {
          potsPos[i] = (pos << 4) | pos;
        }
        else if (pos != previousPos) {
          potsLastposStart[i] = get_tmr10ms();
          potsPos[i] = (pos << 4) | previousStoredPos;
        }
        else if (g_eeGeneral.switchesDelay==SWITCHES_DELAY_NONE || (tmr10ms_t)(get_tmr10ms() - potsLastposStart[i]) > SWITCHES_DELAY()) {
          potsLastposStart[i] = 0;
          potsPos[i] = (pos << 4) | pos;
          if (previousStoredPos != pos) {
            PLAY_SWITCH_MOVED(SWSRC_LAST_SWITCH+i*XPOTS_MULTIPOS_COUNT+pos);
          }
        }
      }
    }
  }
}


getvalue_t getValueForLogicalSwitch(mixsrc_t i)
{
  getvalue_t result = getValue(i);
  if (i>=MIXSRC_FIRST_INPUT && i<=MIXSRC_LAST_INPUT) {
    int8_t trimIdx = virtualInputsTrims[i-MIXSRC_FIRST_INPUT];
    if (trimIdx >= 0) {
      int16_t trim = trims[trimIdx];
      if (trimIdx == THR_STICK && g_model.throttleReversed)
        result -= trim;
      else
        result += trim;
    }
  }
  return result;
}
#else
  #define getValueForLogicalSwitch(i) getValue(i)
#endif

PACK(typedef struct {
  uint8_t state;
  uint8_t last;
}) ls_sticky_struct;

PACK(typedef struct {
  uint16_t state:1;
  uint16_t duration:15;
}) ls_stay_struct;

bool getLogicalSwitch(uint8_t idx)
{
  LogicalSwitchData * ls = lswAddress(idx);
  bool result;

  swsrc_t s = ls->andsw;

  if (ls->func == LS_FUNC_NONE || (s && !getSwitch(s))) {
    if (ls->func != LS_FUNC_STICKY && ls->func != LS_FUNC_EDGE ) {
      // AND switch must not affect STICKY and EDGE processing
      LS_LAST_VALUE(mixerCurrentFlightMode, idx) = CS_LAST_VALUE_INIT;
    }
    result = false;
  }
  else if ((s=lswFamily(ls->func)) == LS_FAMILY_BOOL) {
    bool res1 = getSwitch(ls->v1);
    bool res2 = getSwitch(ls->v2);
    switch (ls->func) {
      case LS_FUNC_AND:
        result = (res1 && res2);
        break;
      case LS_FUNC_OR:
        result = (res1 || res2);
        break;
      // case LS_FUNC_XOR:
      default:
        result = (res1 ^ res2);
        break;
    }
  }
  else if (s == LS_FAMILY_TIMER) {
    result = (LS_LAST_VALUE(mixerCurrentFlightMode, idx) <= 0);
  }
  else if (s == LS_FAMILY_STICKY) {
    result = (LS_LAST_VALUE(mixerCurrentFlightMode, idx) & (1<<0));
  }
  else if (s == LS_FAMILY_EDGE) {
    result = (LS_LAST_VALUE(mixerCurrentFlightMode, idx) & (1<<0));
  }
  else {
    getvalue_t x = getValueForLogicalSwitch(ls->v1);
    getvalue_t y;
    if (s == LS_FAMILY_COMP) {
      y = getValueForLogicalSwitch(ls->v2);

      switch (ls->func) {
        case LS_FUNC_EQUAL:
          result = (x==y);
          break;
        case LS_FUNC_GREATER:
          result = (x>y);
          break;
        default:
          result = (x<y);
          break;
      }
    }
    else {
      mixsrc_t v1 = ls->v1;
#if defined(TELEMETRY_FRSKY)
      // Telemetry
      if (v1 >= MIXSRC_FIRST_TELEM) {
        if (!TELEMETRY_STREAMING() || IS_FAI_FORBIDDEN(v1-1)) {
          result = false;
          goto DurationAndDelayProcessing;
        }

        y = convertLswTelemValue(ls);


      }
      else if (v1 >= MIXSRC_GVAR1) {
        y = ls->v2;
      }
      else {
        y = calc100toRESX(ls->v2);
      }
#else
      if (v1 >= MIXSRC_FIRST_TELEM) {
        y = (int16_t)3 * (128+ls->v2); // it's a Timer
      }
      else if (v1 >= MIXSRC_GVAR1) {
        y = ls->v2; // it's a GVAR
      }
      else {
        y = calc100toRESX(ls->v2);
      }
#endif

      switch (ls->func) {
        case LS_FUNC_VEQUAL:
          result = (x==y);
          break;
        case LS_FUNC_VALMOSTEQUAL:
#if defined(GVARS)
          if (v1 >= MIXSRC_GVAR1 && v1 <= MIXSRC_LAST_GVAR)
            result = (x==y);
          else
#endif
          result = (abs(x-y) < (1024 / STICK_TOLERANCE));
          break;
        case LS_FUNC_VPOS:
          result = (x>y);
          break;
        case LS_FUNC_VNEG:
          result = (x<y);
          break;
        case LS_FUNC_APOS:
          result = (abs(x)>y);
          break;
        case LS_FUNC_ANEG:
          result = (abs(x)<y);
          break;
        default:
        {
          if (LS_LAST_VALUE(mixerCurrentFlightMode, idx) == CS_LAST_VALUE_INIT) {
            LS_LAST_VALUE(mixerCurrentFlightMode, idx) = x;
          }
          int16_t diff = x - LS_LAST_VALUE(mixerCurrentFlightMode, idx);
          bool update = false;
          if (ls->func == LS_FUNC_DIFFEGREATER) {
            if (y >= 0) {
              result = (diff >= y);
              if (diff < 0)
                update = true;
            }
            else {
              result = (diff <= y);
              if (diff > 0)
                update = true;
            }
          }
          else {
            result = (abs(diff) >= y);
          }
          if (result || update) {
            LS_LAST_VALUE(mixerCurrentFlightMode, idx) = x;
          }
          break;
        }
      }
    }
  }

#if defined(TELEMETRY_FRSKY)
DurationAndDelayProcessing:
#endif

    if (ls->delay || ls->duration) {
      LogicalSwitchContext &context = lswFm[mixerCurrentFlightMode].lsw[idx];
      if (result) {
        if (context.timerState == SWITCH_START) {
          // set delay timer
          context.timerState = SWITCH_DELAY;
          context.timer = (ls->func == LS_FUNC_EDGE ? 0 : ls->delay);
        }

        if (context.timerState == SWITCH_DELAY) {
          if (context.timer) {
            result = false;   // return false while delay timer running
          }
          else {
            // set duration timer
            context.timerState = SWITCH_ENABLE;
            context.timer = ls->duration;
          }
        }

        if (context.timerState == SWITCH_ENABLE) {
          result = (ls->duration==0 || context.timer>0); // return false after duration timer runs out
          if (!result && ls->func == LS_FUNC_STICKY) {
            ls_sticky_struct & lastValue = (ls_sticky_struct &)context.lastValue;
            lastValue.state = 0;
          }
        }
      }
      else if (context.timerState == SWITCH_ENABLE && ls->duration > 0 && context.timer > 0) {
        result = true;
      }
      else {
        context.timerState = SWITCH_START;
        context.timer = 0;
      }
    }

  return result;
}

bool getSwitch(swsrc_t swtch, uint8_t flags)
{
  bool result;

  if (swtch == SWSRC_NONE)
    return true;

  uint8_t cs_idx = abs(swtch);

  if (cs_idx == SWSRC_ONE) {
    result = !s_mixer_first_run_done;
  }
  else if (cs_idx == SWSRC_ON) {
    result = true;
  }
  else if (cs_idx <= SWSRC_LAST_SWITCH) {
#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBI6X)
    if (flags & GETSWITCH_MIDPOS_DELAY)
      result = SWITCH_POSITION(cs_idx-SWSRC_FIRST_SWITCH);
    else
      result = switchState(cs_idx-SWSRC_FIRST_SWITCH);
#else
    result = switchState(cs_idx-SWSRC_FIRST_SWITCH);
#endif

  }
#if NUM_XPOTS > 0
  else if (cs_idx <= SWSRC_LAST_MULTIPOS_SWITCH) {
    result = POT_POSITION(cs_idx-SWSRC_FIRST_MULTIPOS_SWITCH);
  }
#endif
  else if (cs_idx <= SWSRC_LAST_TRIM) {
    uint8_t idx = cs_idx - SWSRC_FIRST_TRIM;
    idx = (CONVERT_MODE_TRIMS(idx/2) << 1) + (idx & 1);
    result = trimDown(idx);
  }
#if ROTARY_ENCODERS > 0
  else if (cs_idx == SWSRC_REa) {
    result = REA_DOWN();
  }
#endif
#if ROTARY_ENCODERS > 1
  else if (cs_idx == SWSRC_REb) {
    result = REB_DOWN();
  }
#endif
  else if (cs_idx >= SWSRC_FIRST_SENSOR) {
    result = !telemetryItems[cs_idx-SWSRC_FIRST_SENSOR].isOld();
  }
  else if (cs_idx == SWSRC_TELEMETRY_STREAMING) {
    result = TELEMETRY_STREAMING();
  }
  else if (cs_idx >= SWSRC_FIRST_FLIGHT_MODE) {
#if defined(FLIGHT_MODES)
    uint8_t idx = cs_idx - SWSRC_FIRST_FLIGHT_MODE;
    if (flags & GETSWITCH_MIDPOS_DELAY)
      result = (idx == flightModeTransitionLast);
    else
      result = (idx == mixerCurrentFlightMode);
#else
    result = false;
#endif
   }
  else {
    cs_idx -= SWSRC_FIRST_LOGICAL_SWITCH;
    result = lswFm[mixerCurrentFlightMode].lsw[cs_idx].state;
  }

  return swtch > 0 ? result : !result;
}

/**
  @brief Calculates new state of logical switches for mixerCurrentFlightMode
*/
void evalLogicalSwitches(bool isCurrentFlightmode)
{
  for (unsigned int idx=0; idx<MAX_LOGICAL_SWITCHES; idx++) {
    LogicalSwitchContext & context = lswFm[mixerCurrentFlightMode].lsw[idx];
    bool result = getLogicalSwitch(idx);
    if (isCurrentFlightmode) {
      if (result) {
        if (!context.state) PLAY_LOGICAL_SWITCH_ON(idx);
      }
      else {
        if (context.state) PLAY_LOGICAL_SWITCH_OFF(idx);
      }
    }
    context.state = result;
  }
}

swarnstate_t switches_states = 0;
swsrc_t getMovedSwitch()
{
  static tmr10ms_t s_move_last_time = 0;
  swsrc_t result = 0;

#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBI6X)
  for (int i=0; i<NUM_SWITCHES; i++) {
    if (SWITCH_EXISTS(i)) {
      swarnstate_t mask = ((swarnstate_t)0x03 << (i*2));
      uint8_t prev = (switches_states & mask) >> (i*2);
      uint8_t next = (1024+getValue(MIXSRC_SA+i)) / 1024;
      if (prev != next) {
        switches_states = (switches_states & (~mask)) | ((swarnstate_t)next << (i*2));
        result = 1+(3*i)+next;
      }
    }
  }
#else
  // return delivers 1 to 3 for ID1 to ID3
  // 4..8 for all other switches if changed to true
  // -4..-8 for all other switches if changed to false
  // 9 for Trainer switch if changed to true; Change to false is ignored
  swarnstate_t mask = 0x80;
  for (uint32_t i=NUM_PSWITCH; i>1; i--) {
    bool prev;
    prev = (switches_states & mask);
    // don't use getSwitch here to always get the proper value, even getSwitch manipulates
    bool next = switchState(i-1);
    if (prev != next) {
      if (((i<NUM_PSWITCH) && (i>3)) || next==true)
        result = next ? i : -i;
      if (i<=3 && result==0) result = 1;
      switches_states ^= mask;
    }
    mask >>= 1;
  }
#endif

  if ((tmr10ms_t)(get_tmr10ms() - s_move_last_time) > 10)
    result = 0;

  s_move_last_time = get_tmr10ms();
  return result;
}

#if defined(GUI)
void checkSwitches()
{
  swarnstate_t last_bad_switches = 0xff;
  swarnstate_t states = g_model.switchWarningState;

#if defined(PCBTARANIS) || defined(PCBHORUS)
  uint8_t bad_pots = 0, last_bad_pots = 0xff;
#endif

  while (true) {

#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBI6X)
  #define GETADC_COUNT 1
#endif

#ifdef GETADC_COUNT
    for (uint32_t i=0; i<GETADC_COUNT; i++) {
      GET_ADC_IF_MIXER_NOT_RUNNING();
    }
#undef GETADC_COUNT
#endif

    getMovedSwitch();

    bool warn = false;
#if defined(COLORLCD)
    for (int i=0; i<NUM_SWITCHES; i++) {
      if (SWITCH_WARNING_ALLOWED(i)) {
        unsigned int state = ((states >> (3*i)) & 0x07);
        if (state && state-1 != ((switches_states >> (i*2)) & 0x03)) {
          warn = true;
        }
      }
    }
    if (g_model.potsWarnMode) {
      evalFlightModeMixes(e_perout_mode_normal, 0);
      bad_pots = 0;
      for (int i=0; i<NUM_POTS+NUM_SLIDERS; i++) {
        if (!IS_POT_SLIDER_AVAILABLE(POT1+i)) {
          continue;
        }
        if (!(g_model.potsWarnEnabled & (1 << i)) && (abs(g_model.potsWarnPosition[i] - GET_LOWRES_POT_POSITION(i)) > 1)) {
          warn = true;
          bad_pots |= (1<<i);
        }
      }
    }
#elif defined(PCBTARANIS) || defined(PCBI6X)
    for (int i=0; i<NUM_SWITCHES; i++) {
      if (SWITCH_WARNING_ALLOWED(i) && !(g_model.switchWarningEnable & (1<<i))) {
        swarnstate_t mask = ((swarnstate_t)0x03 << (i*2));
        if (!((states & mask) == (switches_states & mask))) {
          warn = true;
        }
      }
    }
#if !defined(PCBI6X)
    if (g_model.potsWarnMode) {
      evalFlightModeMixes(e_perout_mode_normal, 0);
      bad_pots = 0;
      for (int i=0; i<NUM_POTS+NUM_SLIDERS; i++) {
        if (!IS_POT_SLIDER_AVAILABLE(POT1+i)) {
          continue;
        }
        if (!(g_model.potsWarnEnabled & (1 << i)) && (abs(g_model.potsWarnPosition[i] - GET_LOWRES_POT_POSITION(i)) > 1)) {
          warn = true;
          bad_pots |= (1<<i);
        }
      }
    }
#endif
#else
    for (int i=0; i<NUM_SWITCHES-1; i++) {
      if (!(g_model.switchWarningEnable & (1<<i))) {
      	if (i == 0) {
      	  if ((states & 0x03) != (switches_states & 0x03)) {
      	    warn = true;
      	  }
      	}
        else if ((states & (1<<(i+1))) != (switches_states & (1<<(i+1)))) {
          warn = true;
        }
      }
    }
#endif

    if (!warn) {
      break;
    }

    LED_ERROR_BEGIN();
    resetBacklightTimeout();

    // first - display warning
#if defined(PCBTARANIS) || defined(PCBHORUS)|| defined(PCBI6X)
    if ((last_bad_switches != switches_states) /*|| (last_bad_pots != bad_pots)*/) {
      drawAlertBox(STR_SWITCHWARN, NULL, STR_PRESSANYKEYTOSKIP);
      if (last_bad_switches == 0xff /*|| last_bad_pots == 0xff*/) {
        AUDIO_ERROR_MESSAGE(AU_SWITCH_ALERT);
      }
      int x = SWITCH_WARNING_LIST_X, y = SWITCH_WARNING_LIST_Y;
      int numWarnings = 0;
      for (int i=0; i<NUM_SWITCHES; ++i) {
#if defined(COLORLCD)
        if (SWITCH_WARNING_ALLOWED(i)) {
          unsigned int state = ((g_model.switchWarningState >> (3*i)) & 0x07);
          if (state && state-1 != ((switches_states >> (i*2)) & 0x03)) {
            if (++numWarnings < 6) {
              // LcdFlags attr = ((states & mask) == (switches_states & mask)) ? TEXT_COLOR : ALARM_COLOR;
              LcdFlags attr = ALARM_COLOR;
              drawSwitch(x, y, SWSRC_FIRST_SWITCH+i*3+state-1, attr);
              x += SWITCH_WARNING_LIST_INTERVAL;
            }
            else if (numWarnings == 6) {
              lcdDrawText(x, y, "...", ALARM_COLOR);
            }
          }
        }
#else
        if (SWITCH_WARNING_ALLOWED(i) && !(g_model.switchWarningEnable & (1<<i))) {
          swarnstate_t mask = ((swarnstate_t)0x03 << (i*2));
          LcdFlags attr = ((states & mask) == (switches_states & mask)) ? 0 : INVERS;
          if (attr) {
#if defined(PCBI6X)
            UNUSED(numWarnings);
            char c = "\300-\301"[(states & mask) >> (i*2)];
            drawSource(x, y, MIXSRC_FIRST_SWITCH+i, attr);
            lcdDrawChar(lcdNextPos, y, c, attr);
            x = lcdNextPos + 3;
#else
            if (++numWarnings < 7) {
              char c = "\300-\301"[(states & mask) >> (i*2)];
              drawSource(x, y, MIXSRC_FIRST_SWITCH+i, attr);
              lcdDrawChar(lcdNextPos, y, c, attr);
              x = lcdNextPos + 3;
            }
            else if (numWarnings == 7) {
              lcdDrawText(x, y, "...", 0);
            }
#endif // PCBI6X
          }
        }
#endif
      }
#if !defined(PCBI6X)
      if (g_model.potsWarnMode) {
        if (y == 4*FH+3) {
          y = 6*FH-2;
          x = 60;
        }
        for (int i=0; i<NUM_POTS+NUM_SLIDERS; i++) {
          if (!IS_POT_SLIDER_AVAILABLE(POT1+i)) {
            continue;
          }
          if (!(g_model.potsWarnEnabled & (1 << i))) {
            if (abs(g_model.potsWarnPosition[i] - GET_LOWRES_POT_POSITION(i)) > 1) {
#if defined(COLORLCD)
              char s[8];
              // TODO add an helper
              strncpy(s, &STR_VSRCRAW[1+(NUM_STICKS+1+i)*STR_VSRCRAW[0]], STR_VSRCRAW[0]);
              s[int(STR_VSRCRAW[0])] = '\0';
#else
              lcdDrawTextAtIndex(x, y, STR_VSRCRAW, NUM_STICKS+1+i, INVERS);
              if (IS_POT(POT1+i))
                lcdDrawChar(lcdNextPos, y, g_model.potsWarnPosition[i] > GET_LOWRES_POT_POSITION(i) ? 126 : 127, INVERS);
              else
                lcdDrawChar(lcdNextPos, y, g_model.potsWarnPosition[i] > GET_LOWRES_POT_POSITION(i) ? '\300' : '\301', INVERS);
#endif
#if defined(COLORLCD)
              if (++numWarnings < 6) {
                lcdDrawText(x, y, s, ALARM_COLOR);
              }
              else if (numWarnings == 6) {
                lcdDrawText(x, y, "...", ALARM_COLOR);
              }
              x += 40;
#else
              x = lcdNextPos + 3;
#endif
            }

          }
        }
      }
      last_bad_pots = bad_pots;
#endif // PCBI6X
#else
    if (last_bad_switches != switches_states) {
      RAISE_ALERT(STR_SWITCHWARN, nullptr, STR_PRESSANYKEYTOSKIP, last_bad_switches == 0xff ? AU_SWITCH_ALERT : AU_NONE);
      uint8_t x = 2;
      for (uint32_t i=0; i<NUM_SWITCHES-1; i++) {
        uint8_t attr;
        if (i == 0)
          attr = ((states & 0x03) != (switches_states & 0x03)) ? INVERS : 0;
        else
          attr = (states & (1 << (i+1))) == (switches_states & (1 << (i+1))) ? 0 : INVERS;
        if (!(g_model.switchWarningEnable & (1<<i)))
          drawSwitch(x, 5*FH, (i>0?(i+3):(states&0x3)+1), attr);
        x += 3*FW+FW/2;
      }
#endif

      lcdRefresh();
      lcdSetContrast();
      waitKeysReleased();

      last_bad_switches = switches_states;
    }

    if (pwrCheck() == e_power_off || keyDown()) break;

    checkBacklight();

    wdt_reset();

    RTOS_WAIT_MS(10);
  }

  LED_ERROR_END();
}
#endif // GUI

void logicalSwitchesTimerTick()
{
  for (uint8_t fm=0; fm<MAX_FLIGHT_MODES; fm++) {
    for (uint32_t i=0; i<MAX_LOGICAL_SWITCHES; i++) {
      LogicalSwitchData * ls = lswAddress(i);
      if (ls->func == LS_FUNC_TIMER) {
        int16_t *lastValue = &LS_LAST_VALUE(fm, i);
        if (*lastValue == 0 || *lastValue == CS_LAST_VALUE_INIT) {
          *lastValue = -lswTimerValue(ls->v1);
        }
        else if (*lastValue < 0) {
          if (++(*lastValue) == 0)
            *lastValue = lswTimerValue(ls->v2);
        }
        else { // if (*lastValue > 0)
          *lastValue -= 1;
        }
      }
      else if (ls->func == LS_FUNC_STICKY) {
        ls_sticky_struct & lastValue = (ls_sticky_struct &)LS_LAST_VALUE(fm, i);
        bool before = lastValue.last & 0x01;
        if (lastValue.state) {
          bool now = getSwitch(ls->v2);
          if (now != before) {
            lastValue.last ^= 1;
            if (!before) {
              lastValue.state = 0;
            }
          }
        }
        else {
          bool now = getSwitch(ls->v1);
          if (before != now) {
            lastValue.last ^= 1;
            if (!before) {
              lastValue.state = 1;
            }
          }
        }
      }
      else if (ls->func == LS_FUNC_EDGE) {
        ls_stay_struct & lastValue = (ls_stay_struct &)LS_LAST_VALUE(fm, i);
        // if this ls was reset by the logicalSwitchesReset() the lastValue will be set to CS_LAST_VALUE_INIT(0x8000)
        // when it is unpacked into ls_stay_struct the lastValue.duration will have a value of 0x4000
        // this will produce an instant true for edge logical switch if the second parameter is big enough.
        // So we reset it here.
        if (LS_LAST_VALUE(fm, i) == CS_LAST_VALUE_INIT) {
          lastValue.duration = 0;
        }
        lastValue.state = false;
        bool state = getSwitch(ls->v1);
        if (state) {
          if (ls->v3 == -1 && lastValue.duration == lswTimerValue(ls->v2))
            lastValue.state = true;
          if (lastValue.duration < 1000)
            lastValue.duration++;
        }
        else {
          if (lastValue.duration > lswTimerValue(ls->v2) && (ls->v3 == 0 || lastValue.duration <= lswTimerValue(ls->v2+ls->v3)))
            lastValue.state = true;
          lastValue.duration = 0;
        }
      }

      // decrement delay/duration timer
      LogicalSwitchContext &context = lswFm[fm].lsw[i];
      if (context.timer) {
        context.timer--;
      }
    }
  }
}

LogicalSwitchData * lswAddress(uint8_t idx)
{
  return &g_model.logicalSw[idx];
}

uint8_t lswFamily(uint8_t func)
{
  if (func <= LS_FUNC_ANEG)
    return LS_FAMILY_OFS;
  else if (func <= LS_FUNC_XOR)
    return LS_FAMILY_BOOL;
  else if (func == LS_FUNC_EDGE)
    return LS_FAMILY_EDGE;
  else if (func <= LS_FUNC_LESS)
    return LS_FAMILY_COMP;
  else if (func <= LS_FUNC_ADIFFEGREATER)
    return LS_FAMILY_DIFF;
  else
    return LS_FAMILY_TIMER+func-LS_FUNC_TIMER;
}

int16_t lswTimerValue(delayval_t val)
{
  return (val < -109 ? 129+val : (val < 7 ? (113+val)*5 : (53+val)*10));
}

void logicalSwitchesReset()
{
  memset(lswFm, 0, sizeof(lswFm));

  for (uint8_t fm=0; fm<MAX_FLIGHT_MODES; fm++) {
    for (uint32_t i=0; i<MAX_LOGICAL_SWITCHES; i++) {
      LS_LAST_VALUE(fm, i) = CS_LAST_VALUE_INIT;
    }
  }
}

getvalue_t convertLswTelemValue(LogicalSwitchData * ls)
{
  getvalue_t val;
  val = convert16bitsTelemValue(ls->v1 - MIXSRC_FIRST_TELEM + 1, ls->v2);
  return val;
}

void logicalSwitchesCopyState(uint8_t src, uint8_t dst)
{
  lswFm[dst] = lswFm[src];
}
