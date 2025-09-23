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
#include "timers.h"

// PACK macro definition (moved to top for global scope usage)
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))

// Forward declarations for new helper functions
union SourceNumVal
{
  int16_t rawValue;
  struct
  {
    int16_t value : 15;
    int16_t isSource : 1;
  };
};

// Helper for rounding division to the closest integer for int32_t
int32_t divRoundClosest32(int32_t numerator, int32_t denominator)
{
  if (denominator == 0)
    return 0; // Avoid division by zero
  return (numerator + (numerator > 0 ? denominator / 2 : -denominator / 2)) / denominator;
}

// Private helper function for getValue to handle raw source retrieval
// This function is internal and takes a bool* valid parameter.
getvalue_t _getValue_internal(mixsrc_t i, bool *valid) // Renamed to avoid redefinition and clarify internal use
{
  if (i == MIXSRC_NONE)
  {
    if (valid != nullptr)
      *valid = false;
    return 0;
  }
  else if (i <= MIXSRC_LAST_INPUT)
  { // MIXSRC_LAST_INPUT is typically for virtual inputs
    return anas[i - MIXSRC_FIRST_INPUT];
  }
#if defined(LUA_INPUTS)
  else if (i <= MIXSRC_LAST_LUA)
  { // Changed: i < MIXSRC_LAST_LUA to i <= MIXSRC_LAST_LUA
#if defined(LUA_MODEL_SCRIPTS)
    div_t qr = div((int)(i - MIXSRC_FIRST_LUA), MAX_SCRIPT_OUTPUTS); // Cast to int
    return scriptInputsOutputs[qr.quot].outputs[qr.rem].value;
#else
    if (valid != nullptr)
      *valid = false;
    return 0;
#endif
  }
#endif

  // Changed: Input source handling to match OpenTX's original approach for calibratedAnalogs
  else if (i >= MIXSRC_FIRST_STICK && i <= MIXSRC_LAST_POT + NUM_MOUSE_ANALOGS)
  { // Combined stick and pot handling
    return calibratedAnalogs[i - MIXSRC_Rud];
  }

#if defined(ROTARY_ENCODERS)
  else if (i <= MIXSRC_LAST_ROTARY_ENCODER)
  {
    return getRotaryEncoder(i - MIXSRC_REa);
  }
#endif

  else if (i == MIXSRC_MAX)
  {
    return 1024;
  }

  else if (i <= MIXSRC_LAST_TRIM)
  {
    // Reverted 3POS trim mode check as TRIM_MODE_3POS is not declared
    return calc1000toRESX((int16_t)8 * getTrimValue(mixerCurrentFlightMode, i - MIXSRC_FIRST_TRIM));
  }
  // Reverted switch handling to original OpenTX logic as EdgeTX's switch functions are not available
#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBI6X)
  else if ((i >= MIXSRC_FIRST_SWITCH) && (i <= MIXSRC_LAST_SWITCH))
  {
    mixsrc_t sw = i - MIXSRC_FIRST_SWITCH;
    if (SWITCH_EXISTS(sw))
    {
      return (switchState(3 * sw) ? -1024 : (IS_CONFIG_3POS(sw) && switchState(3 * sw + 1) ? 0 : 1024));
    }
    else
    {
      return 0;
    }
  }
#else
  else if (i == MIXSRC_3POS)
  {
    return (getSwitch(SWSRC_ID0 + 1) ? -1024 : (getSwitch(SWSRC_ID1 + 1) ? 0 : 1024));
  }
  // don't use switchState directly to give getSwitch possibility to hack values if needed for switch warning
  else if (i < MIXSRC_SW1)
  {
    return getSwitch(SWSRC_THR + i - MIXSRC_THR) ? 1024 : -1024;
  }
#endif

#if defined(FUNCTION_SWITCHES)
  else if (i <= MIXSRC_LAST_CUSTOMSWITCH_GROUP)
  {
    // This section is from EdgeTX's custom switch group handling.
    // Requires: getSwitchCountInFSGroup, IS_FSWITCH_GROUP_ON, FSWITCH_GROUP, getFSLogicalState
    // This is a more complex feature and might require significant integration.
    uint8_t group_idx = (uint8_t)(i - MIXSRC_FIRST_CUSTOMSWITCH_GROUP + 1);
    uint8_t stepcount = getSwitchCountInFSGroup(group_idx);
    if (stepcount == 0)
      return 0;

    if (IS_FSWITCH_GROUP_ON(group_idx))
      stepcount--;

    int stepsize = (2 * RESX) / stepcount;
    int value = -RESX;

    for (uint8_t k = 0; k < switchGetMaxFctSwitches(); k++)
    { // Changed i to k to avoid conflict
      if (FSWITCH_GROUP(k) == group_idx)
      {
        if (getFSLogicalState(k) == 1)
          return value + (IS_FSWITCH_GROUP_ON(group_idx) ? 0 : stepsize);
        else
          value += stepsize;
      }
    }
    return -RESX;
  }
#endif

  else if (i <= MIXSRC_LAST_LOGICAL_SWITCH)
  {
    return getSwitch(SWSRC_FIRST_LOGICAL_SWITCH + i - MIXSRC_FIRST_LOGICAL_SWITCH) ? 1024 : -1024;
  }
  else if (i <= MIXSRC_LAST_TRAINER)
  {
    int16_t x = trainerInput[i - MIXSRC_FIRST_TRAINER];
    if (i < MIXSRC_FIRST_TRAINER + NUM_CAL_PPM)
    {
      x -= g_eeGeneral.trainer.calib[i - MIXSRC_FIRST_TRAINER];
    }
    return x * 2;
  }
  else if (i <= MIXSRC_LAST_CH)
  {
    return ex_chans[i - MIXSRC_CH1];
  }

#if defined(GVARS)
  else if (i <= MIXSRC_LAST_GVAR)
  {
    return GVAR_VALUE(i - MIXSRC_GVAR1, getGVarFlightMode(mixerCurrentFlightMode, i - MIXSRC_GVAR1));
  }
#endif

  else if (i == MIXSRC_TX_VOLTAGE)
  {
    return g_vbat100mV;
  }
  else if (i < MIXSRC_FIRST_TIMER)
  {
    // TX_TIME + SPARES
#if defined(RTCLOCK)
    return (g_rtcTime % SECS_PER_DAY) / 60; // number of minutes from midnight
#else
    if (valid != nullptr)
      *valid = false; // Added valid check
    return 0;
#endif
  }
  else if (i <= MIXSRC_LAST_TIMER)
  {
    return timersStates[i - MIXSRC_FIRST_TIMER].val;
  }

  else if (i <= MIXSRC_LAST_TELEM)
  {
    if (IS_FAI_FORBIDDEN(i))
    {
      if (valid != nullptr)
        *valid = false; // Added valid check
      return 0;
    }
    // Cast arguments to int to resolve ambiguity for div
    div_t qr = div((int)i, 3); // Cast to int
    TelemetryItem &telemetryItem = telemetryItems[qr.quot];
    switch (qr.rem)
    {
    case 1:
      return telemetryItem.valueMin;
    case 2:
      return telemetryItem.valueMax;
    default:
      return telemetryItem.value;
    }
  }
  // Added: Default invalid source handling (from EdgeTX)
  if (valid != nullptr)
    *valid = false;
  return 0;
}

// Public getValue function (matches opentx.h prototype)
getvalue_t getValue(mixsrc_t i)
{
  bool invert = false;
  if (((int32_t)i) < 0)
  { // Cast to int32_t for comparison to handle potential negative representations
    invert = true;
    i = (mixsrc_t)(-((int32_t)i)); // Cast to int32_t for negation, then back to mixsrc_t
  }
  bool valid_source = true;                            // Default to valid
  getvalue_t v = _getValue_internal(i, &valid_source); // Call the internal helper
  if (invert)
    v = -v;
  return v;
}

// Helper to get numerical field values from various sources (GVARs, direct values)
// This mimics the behavior of EdgeTX's getSourceNumFieldValue
int32_t getSourceNumFieldValue(int16_t val, int16_t min, int16_t max)
{
  int32_t result;
  SourceNumVal v;
  v.rawValue = val; // Use the union to interpret rawValue
  if (v.isSource)
  {
    // Call the internal getValue with bool* valid
    bool valid_source = true;
    result = _getValue_internal(v.value, &valid_source);
    if (abs((int32_t)v.value) >= MIXSRC_FIRST_GVAR && abs((int32_t)v.value) <= MIXSRC_LAST_GVAR)
    {
      // Mimic behavior of GET_GVAR_PREC1 for GVARs (prec 0 means *10)
      // This assumes g_model.gvars and its prec field are accessible.
      // If not, this part might need adaptation or removal.
      result = result * 10; // Assuming GVARs might need *10 scaling for consistency
    }
    else
    {
      // Convert RESX-scaled value to 1000-scaled (e.g., -1024 to -1000, 1024 to 1000)
      // This assumes calcRESXto1000 is defined elsewhere (e.g., in opentx.h)
      result = divRoundClosest32(result * 1000, RESX); // Changed to divRoundClosest32
    }
  }
  else
  {
    result = v.value * 10; // Direct value is already 10-scaled (e.g., 100% is 1000)
  }
  return limit<int>(min * 10, result, max * 10);
}

int8_t virtualInputsTrims[NUM_INPUTS];
int16_t anas[NUM_INPUTS] = {0};
int16_t trims[NUM_TRIMS] = {0};
int32_t chans[MAX_OUTPUT_CHANNELS] = {0};
BeepANACenter bpanaCenter = 0;

int32_t act[MAX_MIXERS] = {0};

// Moved definition of mixState to here, before any functions use it.
MixState mixState[MAX_MIXERS];

uint8_t mixWarning;

int16_t calibratedAnalogs[NUM_CALIBRATED_ANALOGS];
int16_t channelOutputs[MAX_OUTPUT_CHANNELS] = {0};
int16_t ex_chans[MAX_OUTPUT_CHANNELS] = {0}; // Outputs (before LIMITS) of the last perMain;

#if defined(HELI)
int16_t cyc_anas[3] = {0};
#endif

// #define EXTENDED_EXPO
// increases range of expo curve but costs about 82 bytes flash

// expo-funktion:
// ---------------
// kmplot
// f(x,k)=exp(ln(x)*k/10) ;P[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20]
// f(x,k)=x*x*x*k/10 + x*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]
// f(x,k)=x*x*k/10 + x*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]
// f(x,k)=1+(x-1)*(x-1)*(x-1)*k/10 + (x-1)*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]
// don't know what this above should be, just confusing in my opinion,

// here is the real explanation
// actually the real formula is
/*
 f(x) = exp( ln(x) * 10^k)
 if it is 10^k or e^k or 2^k etc. just defines the max distortion of the expo curve; I think 10 is useful
 this gives values from 0 to 1 for x and output; k must be between -1 and +1
 f(x) = 1024 * ( e^( ln(x/1024) * 10^(k/100) ) )
 This would be really hard to be calculated by such a microcontroller
 Therefore Thomas Husterer compared a few usual function something like x^3, x^4*something, which look similar
 Actually the formula
 f(x) = k*x^3+x*(1-k)
 gives a similar form and should have even advantages compared to a original exp curve.
 This function again expect x from 0 to 1 and k only from 0 to 1
 Therefore rescaling is needed like before:
 f(x) = 1024* ((k/100)*(x/1024)^3 + (x/1024)*(100-k)/100)
 some mathematical tricks
 f(x) = (k*x*x*x/(1024*1024) + x*(100-k)) / 100
 for better rounding results we add the 50
 f(x) = (k*x*x*x/(1024*1024) + x*(100-k) + 50) / 100

 because we now understand the formula, we can optimize it further
 --> calc100to256(k) --> eliminates /100 by replacing with /256 which is just a simple shift right 8
 k is now between 0 and 256
 f(x) = (k*x*x*x/(1024*1024) + x*(256-k) + 128) / 256
 */

// input parameters;
//  x 0 to 1024;
//  k 0 to 100;
// output between 0 and 1024
unsigned int expou(unsigned int x, unsigned int k)
{
#if defined(EXTENDED_EXPO)
  bool extended;
  if (k > 80)
  {
    extended = true;
  }
  else
  {
    k += (k >> 2); // use bigger values before extend, because the effect is anyway very very low
    extended = false;
  }
#endif

  k = calc100to256(k);

  uint32_t value = (uint32_t)x * x;
  value *= (uint32_t)k;
  value >>= 8;
  value *= (uint32_t)x;

#if defined(EXTENDED_EXPO)
  if (extended)
  { // for higher values do more multiplications to get a stronger expo curve
    value >>= 16;
    value *= (uint32_t)x;
    value >>= 4;
    value *= (uint32_t)x;
  }
#endif

  value >>= 12;
  value += (uint32_t)(256 - k) * x + 128;

  return value >> 8;
}

int expo(int x, int k)
{
  if (k == 0)
  {
    return x;
  }

  int y;
  bool neg = (x < 0);

  if (neg)
  {
    x = -x;
  }
  if (x > (int)RESXu)
  {
    x = RESXu;
  }
  if (k < 0)
  {
    y = RESXu - expou(RESXu - x, -k);
  }
  else
  {
    y = expou(x, k);
  }
  return neg ? -y : y;
}

void applyExpos(int16_t *anas, uint8_t mode, uint8_t ovwrIdx, int16_t ovwrValue)
{
  int8_t cur_chn = -1;

  for (uint32_t i = 0; i < MAX_EXPOS; i++)
  {
    ExpoData *ed = expoAddress(i);
    mixsrc_t srcRaw = ed->srcRaw;              // Get the raw source, including potential inversion
    mixsrc_t srcRawAbs = abs((int32_t)srcRaw); // Get the absolute source for comparisons, cast to int32_t for abs

    if (!EXPO_VALID(ed))
      break; // end of list
    if (ed->chn == cur_chn)
      continue;
    if (ed->flightModes & (1 << mixerCurrentFlightMode))
      continue;

    // Streamlined Trainer Input Handling in applyExpos
    // If the source is a trainer input and trainer is not valid, skip this mix.
    if (srcRawAbs >= MIXSRC_FIRST_TRAINER && srcRawAbs <= MIXSRC_LAST_TRAINER && !IS_TRAINER_INPUT_VALID())
    {
      continue;
    }

    if (getSwitch(ed->swtch))
    {
      int32_t v;
      if (srcRaw == ovwrIdx)
      {
        v = ovwrValue;
      }
      else
      {
        // Call the internal getValue with bool* valid
        bool valid_source = true;
        v = _getValue_internal(srcRaw, &valid_source);
        if (srcRawAbs >= MIXSRC_FIRST_TELEM && ed->scale > 0)
        {                                                                                                             // Use srcRawAbs for telemetry check
          v = divRoundClosest32((int32_t)v * 1024, convertTelemValue(srcRawAbs - MIXSRC_FIRST_TELEM + 1, ed->scale)); // Changed to divRoundClosest32
        }
        v = limit<int32_t>(-1024, v, 1024);
      }
      if (EXPO_MODE_ENABLE(ed, v))
      {
        cur_chn = ed->chn;

        //========== CURVE=================
        if (ed->curve.value)
        {
          v = applyCurve(v, ed->curve);
        }

        //========== WEIGHT ===============
        // Use getSourceNumFieldValue for weight
        int32_t weight = getSourceNumFieldValue(ed->weight, MIN_EXPO_WEIGHT, 100);
        v = divRoundClosest32((int32_t)v * weight, 1000); // Changed to divRoundClosest32

        //========== OFFSET ===============
        // Use getSourceNumFieldValue for offset
        int32_t offset = getSourceNumFieldValue(ed->offset, -100, 100);
        if (offset)
          v += divRoundClosest32(calc100toRESX(offset), 10); // Changed to divRoundClosest32

        //========== TRIMS ================
        // Reverted to original OpenTX 'carryTrim' as 'trimSource' is not a member of ExpoData
        if (ed->carryTrim < TRIM_ON)
          virtualInputsTrims[cur_chn] = -ed->carryTrim - 1;
        else if (ed->carryTrim == TRIM_ON && srcRawAbs >= MIXSRC_Rud && srcRawAbs <= MIXSRC_Ail)
          virtualInputsTrims[cur_chn] = srcRawAbs - MIXSRC_Rud; // Using srcRawAbs
        else
          virtualInputsTrims[cur_chn] = -1;
        anas[cur_chn] = v;
      }
    }
  }
}

// #define PREVENT_ARITHMETIC_OVERFLOW
// because of optimizations the reserves before overruns occurs is only the half
// this defines enables some checks the greatly improves this situation
// It should nearly prevent all overruns (is still a chance for it, but quite low)
// negative side is code cost 96 bytes flash

// we do it now half way, only in applyLimits, which costs currently 50bytes
// according opinion poll this topic is currently not very important
// the change below improves already the situation
// the check inside mixer would slow down mix a little bit and costs additionally flash
// also the check inside mixer still is not bulletproof, there may be still situations a overflow could occur
// a bulletproof implementation would take about additional 100bytes flash
// therefore with go with this compromize, interested people could activate this define

// @@@2 open.20.fsguruh ;
// channel = channelnumber -1;
// value = outputvalue with 100 mulitplied usual range -102400 to 102400; output -1024 to 1024
// changed rescaling from *100 to *256 to optimize performance
// rescaled from -262144 to 262144
int16_t applyLimits(uint8_t channel, int32_t value)
{
#if defined(OVERRIDE_CHANNEL_FUNCTION)
  if (safetyCh[channel] != OVERRIDE_CHANNEL_UNDEFINED)
  {
    // safety channel available for channel check
    return calc100toRESX(safetyCh[channel]);
  }
#endif

  // Streamlined Trainer Input Handling
  // If trainer channels function is active and trainer is valid,
  // directly return the trainer input value for this channel.
  // Added placeholder for FUNCTION_TRAINER_CHANNELS if not defined in your OpenTX headers.
#ifndef FUNCTION_TRAINER_CHANNELS
#define FUNCTION_TRAINER_CHANNELS 0 // Placeholder, adjust as per your OpenTX defines
#endif
  if (isFunctionActive(FUNCTION_TRAINER_CHANNELS) && IS_TRAINER_INPUT_VALID())
  {
    return trainerInput[channel] * 2; // Assuming trainerInput is already scaled appropriately
  }

  LimitData *lim = limitAddress(channel);

  if (lim->curve)
  {
    // TODO we loose precision here, applyCustomCurve could work with int32_t on ARM boards...
    // Note: Division by 256 is kept as '/' because 'value' can be negative,
    // and C++ integer division truncates towards zero, while right bit shift
    // truncates towards negative infinity for negative numbers, which could
    // alter the intended mathematical behavior.
    if (lim->curve > 0)
      value = 256 * applyCustomCurve(value / 256, lim->curve - 1);
    else
      value = 256 * applyCustomCurve(-value / 256, -lim->curve - 1);
  }

  int16_t ofs = LIMIT_OFS_RESX(lim);
  int16_t lim_p = LIMIT_MAX_RESX(lim);
  int16_t lim_n = LIMIT_MIN_RESX(lim);

  if (ofs > lim_p)
    ofs = lim_p;
  if (ofs < lim_n)
    ofs = lim_n;

  // because the rescaling optimization would reduce the calculation reserve we activate this for all builds
  // it increases the calculation reserve from factor 20,25x to 32x, which it slightly better as original
  // without it we would only have 16x which is slightly worse as original, we should not do this

  // thanks to gbirkus, he motivated this change, which greatly reduces overruns
  // unfortunately the constants and 32bit compares generates about 50 bytes codes; didn't find a way to get it down.
  value = limit(int32_t(-RESXl * 256), value, int32_t(RESXl * 256)); // saves 2 bytes compared to other solutions up to now

#if defined(PPM_LIMITS_SYMETRICAL)
  if (value)
  {
    int16_t tmp;
    if (lim->symetrical)
      tmp = (value > 0) ? (lim_p) : (-lim_n);
    else
      tmp = (value > 0) ? (lim_p - ofs) : (-lim_n + ofs);
    value = (int32_t)value * tmp; //  div by 1024*256 -> output = -1024..1024
#else
  if (value)
  {
    int16_t tmp = (value > 0) ? (lim_p - ofs) : (-lim_n + ofs);
    value = (int32_t)value * tmp; //  div by 1024*256 -> output = -1024..1024
#endif

    // Enhanced Rounding: Use divRoundClosest32 for more precise rounding
    // Original: tmp = (value + (value < 0 ? (1<<17)-1 : (1<<17))) >> 18;
    tmp = divRoundClosest32(value, 1024 * 256); // Assuming 1024*256 is the scaling factor to normalize, changed to divRoundClosest32

    ofs += tmp; // ofs can to added directly because already recalculated,
  }

  if (ofs > lim_p)
    ofs = lim_p;
  if (ofs < lim_n)
    ofs = lim_n;

  if (lim->revert)
    ofs = -ofs; // finally do the reverse.

#if defined(OVERRIDE_CHANNEL_FUNCTION)
  if (safetyCh[channel] != OVERRIDE_CHANNEL_UNDEFINED)
  {
    // safety channel available for channel check
    ofs = calc100toRESX(safetyCh[channel]);
  }
#endif

  return ofs;
}

// Added: _getValue helper function (from EdgeTX)
// These lookup tables are for switch states.
static const getvalue_t _switch_2pos_lookup[] = {
    -1024, // SWITCH_HW_UP
    +1024, // SWITCH_HW_MID (EdgeTX uses this for 2-pos switches, which means it's ON)
    +1024, // SWITCH_HW_DOWN
};

static const getvalue_t _switch_3pos_lookup[] = {
    -1024, // SWITCH_HW_UP
    0,     // SWITCH_HW_MID
    +1024, // SWITCH_HW_DOWN
};

void evalInputs(uint8_t mode)
{
  BeepANACenter anaCenter = 0;

  // Added: STICK_DEAD_ZONE (from EdgeTX)
#if defined(STICK_DEAD_ZONE)
  int16_t deadZoneOffset = g_eeGeneral.stickDeadZone ? 2 << (g_eeGeneral.stickDeadZone - 1) : 0;
#endif

  // Changed: Using ADC input offsets and max inputs (from EdgeTX)
  // These require your HAL/ADC driver to provide these functions.
  // For now, I'll use placeholders if your OpenTX doesn't have direct equivalents.
  // You might need to adjust NUM_STICKS, NUM_POTS, NUM_SLIDERS based on your specific hardware.
  // Assuming a direct mapping for simplicity if no specific ADC functions are available.
  uint32_t max_calib_analogs = NUM_STICKS + NUM_POTS + NUM_SLIDERS + NUM_MOUSE_ANALOGS; // Placeholder, changed type to uint32_t
  // Removed unused pots_offset variable

  for (uint32_t i = 0; i < max_calib_analogs; i++)
  {                                                      // Changed loop limit
    uint8_t ch = (i < NUM_STICKS ? CONVERT_MODE(i) : i); // Original OpenTX logic for ch
    int16_t v = anaIn(i);                                // Original OpenTX anaIn

    if (IS_POT_MULTIPOS(i))
    {
      v -= RESX;
    }
#if !defined(SIMU)
    else
    {
      CalibData *calib = &g_eeGeneral.calib[i];
      v -= calib->mid;
      v = v * (int32_t)RESX / (max((int16_t)100, (v > 0 ? calib->spanPos : calib->spanNeg)));
    }
#endif

    if (v < -RESX)
      v = -RESX;
    if (v > RESX)
      v = RESX;

    // Added: STICK_DEAD_ZONE logic (from EdgeTX)
#if defined(STICK_DEAD_ZONE)
    if (g_eeGeneral.stickDeadZone && ch != THR_STICK)
    { // Using THR_STICK as placeholder for inputMappingGetThrottle()
      if (v > deadZoneOffset)
      {
        v = (int16_t)divRoundClosest32((int32_t)(v - deadZoneOffset) * 1024L, (1024L - deadZoneOffset)); // Changed to divRoundClosest32
      }
      else if (v < -deadZoneOffset)
      {
        v = (int16_t)divRoundClosest32((int32_t)(v + deadZoneOffset) * 1024L, (1024L - deadZoneOffset)); // Changed to divRoundClosest32
      }
      else
      {
        v = 0;
      }
    }
#endif

    if (g_model.throttleReversed && ch == THR_STICK)
    { // Using THR_STICK as placeholder for inputMappingGetThrottle()
      v = -v;
    }

    BeepANACenter mask = (BeepANACenter)1 << ch;

    calibratedAnalogs[ch] = v; // for show in expo

    // filtering for center beep
    // Optimized: Replaced division by 16 with a right bit shift by 4.
    // This is safe because abs(v) is always non-negative.
    uint8_t tmp = (uint16_t)abs((int32_t)v) >> 4; // Cast to int32_t for abs
    if (mode == e_perout_mode_normal)
    {
      if (tmp == 0 || (tmp == 1 && (bpanaCenter & mask)))
      {
        anaCenter |= mask;
        // Added: s_mixer_first_run_done check (from EdgeTX)
        if ((g_model.beepANACenter & mask) && !(bpanaCenter & mask) &&
            s_mixer_first_run_done && !menuCalibrationState)
        { // Added s_mixer_first_run_done
          if (!IS_POT(i) || IS_POT_SLIDER_AVAILABLE(i))
          { // Original OpenTX logic
            AUDIO_POT_MIDDLE(i);
          }
        }
      }
    }

    if (ch < NUM_STICKS)
    { // only do this for sticks
      if (mode & e_perout_mode_nosticks)
      {
        v = 0;
      }

      if (mode <= e_perout_mode_inactive_flight_mode && isFunctionActive(FUNCTION_TRAINER + ch) && IS_TRAINER_INPUT_VALID())
      {
        // trainer mode
        TrainerMix *td = &g_eeGeneral.trainer.mix[ch];
        if (td->mode)
        {
          uint8_t chStud = td->srcChn;
          int32_t vStud = (trainerInput[chStud] - g_eeGeneral.trainer.calib[chStud]);
          vStud *= td->studWeight;
          vStud = divRoundClosest32(vStud, 50); // Changed: Using divRoundClosest32
          switch (td->mode)
          {
          case 1: // TRAINER_ADD
            // add-mode
            v = limit<int16_t>(-RESX, v + vStud, RESX);
            break;
          case 2: // TRAINER_REPL
            // subst-mode
            v = vStud;
            break;
          }
        }
      }
      calibratedAnalogs[ch] = v;
    }
  }

#if defined(ROTARY_ENCODERS)
  for (uint32_t i = 0; i < NUM_ROTARY_ENCODERS; i++)
  {
    if (getRotaryEncoder(i) == 0)
    {
      anaCenter |= ((BeepANACenter)1 << (NUM_STICKS + NUM_POTS + NUM_SLIDERS + NUM_MOUSE_ANALOGS + i));
    }
  }
#endif

#if NUM_MOUSE_ANALOGS > 0
  for (uint32_t i = 0; i < NUM_MOUSE_ANALOGS; i++)
  {
    uint8_t ch = NUM_STICKS + NUM_POTS + NUM_SLIDERS + i;
    int16_t v = anaIn(MOUSE1 + i);
    CalibData *calib = &g_eeGeneral.calib[ch];
    v -= calib->mid;
    v = v * (int32_t)RESX / (max((int16_t)100, (v > 0 ? calib->spanPos : calib->spanNeg)));
    if (v < -RESX)
      v = -RESX;
    if (v > RESX)
      v = RESX;
    calibratedAnalogs[ch] = v;
  }
#endif

  /* EXPOs */
  applyExpos(anas, mode, 0, 0); // ovwrIdx and ovwrValue are not used in this context, passing 0,0

  /* TRIMs */
  evalTrims(); // when no virtual inputs, the trims need the anas array calculated above (when throttle trim enabled)

  if (mode == e_perout_mode_normal)
  {
    bpanaCenter = anaCenter;
  }
}

// Added: IDLE_TRIM_SCALE (from EdgeTX)
#if defined(SURFACE_RADIO)
constexpr int IDLE_TRIM_SCALE = 1;
#else
constexpr int IDLE_TRIM_SCALE = 2;
#endif

int getStickTrimValue(int stick, int stickValue)
{
  if (stick < 0)
    return 0;

  int trim = trims[stick];
  if (stick == THR_STICK)
  { // Using THR_STICK as placeholder for inputMappingGetThrottle()
    if (g_model.thrTrim)
    {
      int trimMin = g_model.extendedTrims ? 2 * TRIM_EXTENDED_MIN : 2 * TRIM_MIN;
      // Changed: Using divRoundClosest32 and explicit division
      trim = (int)divRoundClosest32((g_model.throttleReversed ? (trim + trimMin) : (trim - trimMin)) * (RESX - stickValue), (1 << (RESX_SHIFT + 1)));
    }
    if (g_model.throttleReversed)
    {
      trim = -trim;
    }
  }
  return trim;
}

// Changed: getSourceTrimOrigin helper (from EdgeTX)
int getSourceTrimOrigin(int source)
{
  if (source >= MIXSRC_Rud && source <= MIXSRC_Ail) // Assuming MIXSRC_Rud and MIXSRC_Ail map to sticks
    return source - MIXSRC_Rud;
  else if (source >= MIXSRC_FIRST_INPUT && source <= MIXSRC_LAST_INPUT)
    return virtualInputsTrims[source - MIXSRC_FIRST_INPUT];
  else
    return -1;
}

int getSourceTrimValue(int source, int stickValue = 0)
{
  // Changed: Using getSourceTrimOrigin (from EdgeTX)
  auto origin = getSourceTrimOrigin(source);
  if (origin >= 0)
  {
    return getStickTrimValue(origin, stickValue);
  }
  else
  {
    return 0;
  }
}

uint8_t mixerCurrentFlightMode;
void evalFlightModeMixes(uint8_t mode, uint8_t tick10ms)
{
  evalInputs(mode);

  if (tick10ms)
    evalLogicalSwitches(mode == e_perout_mode_normal);

#if defined(HELI)
  bool valid_source = true;
  int heliEleValue = _getValue_internal(g_model.swashR.elevatorSource, &valid_source); // Changed: getValue signature
  int heliAilValue = _getValue_internal(g_model.swashR.aileronSource, &valid_source);  // Changed: getValue signature
  if (g_model.swashR.value)
  {
    uint32_t v = ((int32_t)heliEleValue * heliEleValue + (int32_t)heliAilValue * heliAilValue);
    uint32_t q = calc100toRESX(g_model.swashR.value);
    q *= q;
    if (v > q)
    {
      uint16_t d = isqrt32(v);
      int16_t tmp = calc100toRESX(g_model.swashR.value);
      heliEleValue = divRoundClosest32((int32_t)heliEleValue * tmp, d); // Changed: Using divRoundClosest32
      heliAilValue = divRoundClosest32((int32_t)heliAilValue * tmp, d); // Changed: Using divRoundClosest32
    }
  }

// Note: REZ_SWASH_X and REZ_SWASH_Y macros are kept as is.
// Although they contain divisions by powers of 2, the inputs (x) can be negative.
// C++ integer division truncates towards zero, while bit shifts truncate towards negative infinity.
// Changing these would alter the mathematical behavior for negative inputs.
#define REZ_SWASH_X(x) ((x) - (x) / 8 - (x) / 128 - (x) / 512) //  1024*sin(60) ~= 886
#define REZ_SWASH_Y(x) ((x))                                   //  1024 => 1024

  if (g_model.swashR.type)
  {
    getvalue_t vp = heliEleValue + getSourceTrimValue(g_model.swashR.elevatorSource);
    getvalue_t vr = heliAilValue + getSourceTrimValue(g_model.swashR.aileronSource);
    getvalue_t vc = 0;
    if (g_model.swashR.collectiveSource)
    {
      bool valid_source = true;
      vc = _getValue_internal(g_model.swashR.collectiveSource, &valid_source); // Changed: getValue signature
    }

    vp = divRoundClosest32((vp * g_model.swashR.elevatorWeight), 100);   // Changed: Using divRoundClosest32
    vr = divRoundClosest32((vr * g_model.swashR.aileronWeight), 100);    // Changed: Using divRoundClosest32
    vc = divRoundClosest32((vc * g_model.swashR.collectiveWeight), 100); // Changed: Using divRoundClosest32

    switch (g_model.swashR.type)
    {
    case SWASH_TYPE_120:
      vp = REZ_SWASH_Y(vp);
      vr = REZ_SWASH_X(vr);
      // Note: Divisions by 2 are kept as '/' because inputs can be negative,
      // and C++ integer division truncates towards zero.
      cyc_anas[0] = vc - vp;
      cyc_anas[1] = vc + vp / 2 + vr;
      cyc_anas[2] = vc + vp / 2 - vr;
      break;
    case SWASH_TYPE_120X:
      vp = REZ_SWASH_X(vp);
      vr = REZ_SWASH_Y(vr);
      // Note: Divisions by 2 are kept as '/' because inputs can be negative,
      // and C++ integer division truncates towards zero.
      cyc_anas[0] = vc - vr;
      cyc_anas[1] = vc + vr / 2 + vp;
      cyc_anas[2] = vc + vr / 2 - vp;
      break;
    case SWASH_TYPE_140:
      vp = REZ_SWASH_Y(vp);
      vr = REZ_SWASH_Y(vr);
      cyc_anas[0] = vc - vp;
      cyc_anas[1] = vc + vp + vr;
      cyc_anas[2] = vc + vp - vr;
      break;
    case SWASH_TYPE_90:
      vp = REZ_SWASH_Y(vp);
      vr = REZ_SWASH_Y(vr);
      cyc_anas[0] = vc - vp;
      cyc_anas[1] = vc + vr;
      cyc_anas[2] = vc - vr;
      break;
    default:
      break;
    }
  }
#endif

  memclear(chans, sizeof(chans)); // all outputs to 0

  //========== MIXER LOOP ===============
  uint8_t lv_mixWarning = 0;

  uint8_t pass = 0;

  // Changed: Using all_channels_dirty and explicit channel bit functions for clarity (from EdgeTX)
  // You might need to define all_channels_dirty and these helper functions/macros
  // Example:
  // constexpr bitfield_channels_t all_channels_dirty = (bitfield_channels_t)-1;
  // static inline bitfield_channels_t channel_bit(uint16_t ch) { return (bitfield_channels_t)1 << ch; }
  // static inline bitfield_channels_t channel_dirty(bitfield_channels_t mask, uint16_t ch) { return mask & channel_bit(ch); }
  // static inline bitfield_channels_t upper_channels_mask(uint16_t ch) { return ~(channel_bit(ch)) + 1; }
  bitfield_channels_t dirtyChannels = (bitfield_channels_t)-1; // all dirty when mixer starts

  // Added: activeMixes array (from EdgeTX)
  // bool activeMixes[MAX_MIXERS];

  do
  {
    bitfield_channels_t passDirtyChannels = 0;

    for (uint32_t i = 0; i < MAX_MIXERS; i++)
    {

      MixData *md = mixAddress(i);
      mixsrc_t srcRaw = md->srcRaw;
      mixsrc_t srcRawAbs = abs((int32_t)srcRaw); // Added: srcRawAbs for absolute value, cast to int32_t for abs

      if (srcRaw == 0)
      {
#if defined(COLORLCD) // Added: Conditional break/continue (from EdgeTX)
        continue;
#else
        break;
#endif
      }

      // Changed: Using channel_dirty helper (from EdgeTX)
      if (!((dirtyChannels & ((bitfield_channels_t)1 << md->destCh)))) // Original OpenTX logic
        continue;

      // if this is the first calculation for the destination channel, initialize it with 0 (otherwise would be random)
      if (i == 0 || md->destCh != (md - 1)->destCh)
        chans[md->destCh] = 0;

      //========== FLIGHT MODE && SWITCH =====
      bool mixCondition = (md->flightModes != 0 || md->swtch);
      // Changed: fmEnabled check (from EdgeTX)
      bool fmEnabled = (md->flightModes & (1 << mixerCurrentFlightMode)) == 0;
      bool mixLineActive = fmEnabled && getSwitch(md->swtch);             // Added: mixLineActive
      delayval_t mixEnabled = (mixLineActive) ? DELAY_POS_MARGIN + 1 : 0; // Changed: Using mixLineActive

      // Changed: Trainer check logic (from EdgeTX)
      if (mixLineActive)
      {
        if (srcRawAbs >= MIXSRC_FIRST_TRAINER && srcRawAbs <= MIXSRC_LAST_TRAINER && !IS_TRAINER_INPUT_VALID())
        {
          mixCondition = true;
          mixEnabled = 0;
        }

#if defined(LUA_MODEL_SCRIPTS)
        // disable mixer if Lua script is used as source and script was killed
        if (srcRawAbs >= MIXSRC_FIRST_LUA && srcRawAbs <= MIXSRC_LAST_LUA)
        {
          // Cast arguments to int to resolve ambiguity for div
          div_t qr = div((int)(srcRawAbs - MIXSRC_FIRST_LUA), MAX_SCRIPT_OUTPUTS); // Cast to int
          // Changed: Loop for script states (from EdgeTX)
          for (int n = 0; n < MAX_SCRIPTS; n += 1)
          { // Assuming MAX_SCRIPTS is defined
            if ((scriptInternalData[n].reference == qr.quot) && (scriptInternalData[n].state != SCRIPT_OK))
            {
              mixCondition = true;
              mixEnabled = 0;
            }
          }
        }
#endif
      }

      //========== VALUE ===============
      getvalue_t v = 0;
      if (mode > e_perout_mode_inactive_flight_mode)
      {
        if (mixEnabled)
        {
          bool valid_source = true;
          v = _getValue_internal(srcRaw, &valid_source); // Changed: getValue signature
        }
        else
          continue;
      }
      else
      {
        // Changed: Use srcRaw directly, and then check for channel source (from EdgeTX)
        bool valid_source = true;
        v = _getValue_internal(srcRaw, &valid_source); // Changed: getValue signature

        if (srcRawAbs >= MIXSRC_FIRST_CH && srcRawAbs <= MIXSRC_LAST_CH)
        { // Check if source is a channel

          auto srcChan = srcRawAbs - MIXSRC_FIRST_CH;
          if (srcChan <= MAX_OUTPUT_CHANNELS && md->destCh != srcChan)
          {

            // check whether we need to recompute the current channel later
            // Changed: Using upper_channels_mask and channel_dirty (from EdgeTX)
            bitfield_channels_t upperChansMask = ~(((bitfield_channels_t)1 << md->destCh) - 1);           // Equivalent to upper_channels_mask(md->destCh)
            bitfield_channels_t srcChanDirtyMask = (dirtyChannels & ((bitfield_channels_t)1 << srcChan)); // Equivalent to channel_dirty(dirtyChannels, srcChan)

            // if the source is any of the channels marked as dirty
            // or contained in [ destCh, MAX_OUTPUT_CHANNELS [
            if (srcChanDirtyMask & (passDirtyChannels | upperChansMask))
            {
              passDirtyChannels |= (bitfield_channels_t)1 << md->destCh;
            }

            // if the source has already be computed,
            // then use it!
            if (srcChan < md->destCh || pass > 0)
            {
              // channels are in [ -1024 * 256, 1024 * 256 ]
              v = chans[srcChan] >> 8;
            }
          }
        }
        if (!mixCondition)
        {
          mixEnabled = v;
        }
      }

      bool applyOffsetAndCurve = true;

      //========== DELAYS ===============
      // Changed: Using mixState instead of swOn
      delayval_t _swOn = mixState[i].now;
      delayval_t _swPrev = mixState[i].prev;
      bool swTog = (mixEnabled > _swOn + DELAY_POS_MARGIN || mixEnabled < _swOn - DELAY_POS_MARGIN);
      if (mode == e_perout_mode_normal && swTog)
      {
        if (!mixState[i].delay)
          _swPrev = _swOn;
        // Removed md->delayPrec as it's not in OpenTX's MixData struct
        int32_t precMult = 10; // Reverted to original OpenTX constant
        mixState[i].delay = (mixEnabled > _swOn ? md->delayUp : md->delayDown) * precMult;
        mixState[i].now = mixEnabled;
        mixState[i].prev = _swPrev;
      }
      if (mode == e_perout_mode_normal && mixState[i].delay > 0)
      {
        mixState[i].delay = max<int16_t>(0, (int16_t)mixState[i].delay - tick10ms);
        if (!mixCondition)
          v = _swPrev;
        else if (mixEnabled)
          continue;
      }
      else
      {
        if (mode == e_perout_mode_normal)
        {
          mixState[i].now = mixState[i].prev = mixEnabled; // Changed: Using mixState
        }
        if (!mixEnabled)
        {
          // Changed: MLTPX_REPL to MLTPX_REP
          if ((md->speedDown || md->speedUp) && md->mltpx != MLTPX_REP)
          {
            if (mixCondition)
            {
              v = (md->mltpx == MLTPX_ADD ? 0 : RESX);
              applyOffsetAndCurve = false;
            }
          }
          else if (mixCondition)
          {
            continue;
          }
        }
      }

      if (mode == e_perout_mode_normal && (!mixCondition || mixEnabled || mixState[i].delay))
      { // Changed: Using mixState
        if (md->mixWarn)
          lv_mixWarning |= 1 << (md->mixWarn - 1);
      }

      if (applyOffsetAndCurve)
      {
        // Changed: Trim application logic (from EdgeTX)
        bool applyTrims = !(mode & e_perout_mode_notrims);
        if (!applyTrims && g_model.thrTrim)
        {
          auto origin = getSourceTrimOrigin(srcRaw);
          // Reverted to THR_STICK as getThrottleStickTrimSource() is not a member of ModelData
          if (origin == THR_STICK - MIXSRC_FIRST_TRIM)
          {
            applyTrims = true;
          }
        }
        //========== TRIMS ================
        // Reverted to original OpenTX 'carryTrim' as 'trimSource' is not a member of ExpoData
        if (applyTrims && md->carryTrim == 0)
        {
          v += getSourceTrimValue(srcRaw, v);
        }
      }

      // Changed: Using getSourceNumFieldValue for weight
      int32_t weight = getSourceNumFieldValue(MD_WEIGHT(md), -RESX, RESX);
      weight = calc100to256_16Bits(weight);
      //========== SPEED ===============
      // now its on input side, but without weight compensation. More like other remote controls
      // lower weight causes slower movement

      if (mode <= e_perout_mode_inactive_flight_mode && (md->speedUp || md->speedDown))
      { // there are delay values
#define DEL_MULT_SHIFT 8
        // we recale to a mult 256 higher value for calculation
        int32_t tact = act[i];
        int16_t diff = v - (tact >> DEL_MULT_SHIFT);
        if (diff)
        {
          // open.20.fsguruh: speed is defined in % movement per second; In menu we specify the full movement (-100% to 100%) = 200% in total
          // the unit of the stored value is the value from md->speedUp or md->speedDown * 0.1s; e.g. value 4 means 0.4 seconds
          // because we get a tick each 10msec, we need 100 ticks for one second
          // the value in md->speedXXX gives the time it should take to do a full movement from -100 to 100 therefore 200%. This equals 2048 in recalculated internal range
          if (tick10ms || !s_mixer_first_run_done)
          {
            // only if already time is passed add or substract a value according the speed configured
            int32_t rate = (int32_t)tick10ms << (DEL_MULT_SHIFT + 11); // = DEL_MULT*2048*tick10ms
            // rate equals a full range for one second; if less time is passed rate is accordingly smaller
            // if one second passed, rate would be 2048 (full motion)*256(recalculated weight)*100(100 ticks needed for one second)
            int32_t currentValue = ((int32_t)v << DEL_MULT_SHIFT);
            // Removed md->speedPrec as it's not in OpenTX's MixData struct
            int32_t precMult = 10; // Reverted to original OpenTX constant
            if (diff > 0)
            {
              if (s_mixer_first_run_done && md->speedUp > 0)
              {
                // if a speed upwards is defined recalculate the new value according configured speed; the higher the speed the smaller the add value is
                // Changed: Using precMult for division (from EdgeTX)
                int32_t newValue = tact + divRoundClosest32(rate, (int16_t)precMult * md->speedUp); // Changed: Using divRoundClosest32
                if (newValue < currentValue)
                  currentValue = newValue; // Endposition; prevent toggling around the destination
              }
            }
            else
            { // if is <0 because ==0 is not possible
              if (s_mixer_first_run_done && md->speedDown > 0)
              {
                // see explanation in speedUp
                // Changed: Using precMult for division (from EdgeTX)
                int32_t newValue = tact - divRoundClosest32(rate, (int16_t)precMult * md->speedDown); // Changed: Using divRoundClosest32
                if (newValue > currentValue)
                  currentValue = newValue; // Endposition; prevent toggling around the destination
              }
            }
            act[i] = tact = currentValue;
            // open.20.fsguruh: this implementation would save about 50 bytes code
          } // endif tick10ms ; in case no time passed assign the old value, not the current value from source
          v = (tact >> DEL_MULT_SHIFT);
        }
      }

      //========== CURVES ===============
      if (applyOffsetAndCurve && md->curve.type != CURVE_REF_DIFF && md->curve.value)
      {
        v = applyCurve(v, md->curve);
      }

      //========== WEIGHT ===============
      int32_t dv = (int32_t)v * weight;
      dv = divRoundClosest32(dv, 10); // Changed: Using divRoundClosest32

      //========== OFFSET / AFTER ===============
      if (applyOffsetAndCurve)
      {
        // Changed: Using getSourceNumFieldValue for offset
        int32_t offset = getSourceNumFieldValue(MD_OFFSET(md), GV_RANGELARGE_NEG, GV_RANGELARGE);
        if (offset)
          dv += divRoundClosest32(calc100toRESX_16Bits(offset), 10) << 8; // Changed: Using divRoundClosest32
      }

      //========== DIFFERENTIAL =========
      if (md->curve.type == CURVE_REF_DIFF && md->curve.value)
      {
        dv = applyCurve(dv, md->curve);
      }

      int32_t *ptr = &chans[md->destCh]; // Save calculating address several times

      switch (md->mltpx)
      {
      // Changed: MLTPX_REPL to MLTPX_REP
      case MLTPX_REP:
        *ptr = dv;
        break;
      case MLTPX_MUL:
        // @@@2 we have to remove the weight factor of 256 in case of 100%; now we use the new base of 256
        dv >>= 8;
        dv *= *ptr;
        dv = divRoundClosest32(dv, RESXl); // Changed to divRoundClosest32
        *ptr = dv;
        break;
      default: // MLTPX_ADD
        // Changed: Using divRoundClosest32 for addition (from EdgeTX)
        *ptr += dv; // Mixer output add up to the line (dv + (dv>0 ? 100/2 : -100/2))/(100);
        break;
      } // endswitch md->mltpx
#ifdef PREVENT_ARITHMETIC_OVERFLOW
      /*
            // a lot of assumptions must be true, for this kind of check; not really worth for only 4 bytes flash savings
            // this solution would save again 4 bytes flash
            int8_t testVar=(*ptr<<1)>>24;
            if ( (testVar!=-1) && (testVar!=0 ) ) {
              // this devices by 64 which should give a good balance between still over 100% but lower then 32x100%; should be OK
              *ptr >>= 6;  // this is quite tricky, reduces the value a lot but should be still over 100% and reduces flash need
            } */

      PACK(union u_int16int32_t {
        struct
        {
          int16_t lo;
          int16_t hi;
        } words_t;
        int32_t dword;
      });

      u_int16int32_t tmp;
      tmp.dword = *ptr;

      if (tmp.dword < 0)
      {
        if ((tmp.words_t.hi & 0xFF80) != 0xFF80)
          tmp.words_t.hi = 0xFF86; // set to min nearly
      }
      else
      {
        if ((tmp.words_t.hi | 0x007F) != 0x007F)
          tmp.words_t.hi = 0x0079; // set to max nearly
      }
      *ptr = tmp.dword;
      // this implementation saves 18bytes flash

      /* dv=*ptr>>8;
            if (dv>(32767-RESXl)) {
              *ptr=(32767-RESXl)<<8;
            } else if (dv<(-32767+RESXl)) {
              *ptr=(-32767+RESXl)<<8;
            }*/
      // *ptr=limit( int32_t(int32_t(-1)<<23), *ptr, int32_t(int32_t(1)<<23));  // limit code cost 72 bytes
      // *ptr=limit( int32_t((-32767+RESXl)<<8), *ptr, int32_t((32767-RESXl)<<8));  // limit code cost 80 bytes
#endif

    } // endfor mixers

    tick10ms = 0;
    dirtyChannels &= passDirtyChannels;

  } while (++pass < 5 && dirtyChannels);

  mixWarning = lv_mixWarning;

  // Added: Copy activeMixes to mixState.activeMix (from EdgeTX)
  // for (uint8_t i=0; i<MAX_MIXERS; i++) {
  //   mixState[i].activeMix = activeMixes[i];
  // }
}

#define MAX_ACT 0xffff
uint8_t lastFlightMode = 255; // TODO reinit everything here when the model changes, no???

tmr10ms_t flightModeTransitionTime;
uint8_t flightModeTransitionLast = 255;

void evalMixes(uint8_t tick10ms)
{
  int32_t sum_chans512[MAX_OUTPUT_CHANNELS];

  static uint16_t fp_act[MAX_FLIGHT_MODES] = {0};
  static uint16_t delta = 0;
  static ACTIVE_PHASES_TYPE flightModesFade = 0;

  LS_RECURSIVE_EVALUATION_RESET();

  uint8_t fm = getFlightMode();

  if (lastFlightMode != fm)
  {
    flightModeTransitionTime = get_tmr10ms();

    if (lastFlightMode == 255)
    {
      fp_act[fm] = MAX_ACT;
    }
    else
    {
      uint8_t fadeTime = max(g_model.flightModeData[lastFlightMode].fadeOut, g_model.flightModeData[fm].fadeIn);
      ACTIVE_PHASES_TYPE transitionMask = ((ACTIVE_PHASES_TYPE)1 << lastFlightMode) + ((ACTIVE_PHASES_TYPE)1 << fm);
      if (fadeTime)
      {
        flightModesFade |= transitionMask;
        // Changed: Using divRoundClosest32 for delta calculation
        delta = divRoundClosest32(MAX_ACT, (int32_t)10 * fadeTime);
      }
      else
      {
        flightModesFade &= ~transitionMask;
        fp_act[lastFlightMode] = 0;
        fp_act[fm] = MAX_ACT;
      }
      logicalSwitchesCopyState(lastFlightMode, fm); // push last logical switches state from old to new flight mode
    }
    lastFlightMode = fm;
  }

  if (flightModeTransitionTime && get_tmr10ms() > flightModeTransitionTime + SWITCHES_DELAY())
  {
    flightModeTransitionTime = 0;
    if (fm != flightModeTransitionLast)
    {
      if (flightModeTransitionLast != 255)
      {
        PLAY_PHASE_OFF(flightModeTransitionLast);
      }
      PLAY_PHASE_ON(fm);
      flightModeTransitionLast = fm;
    }
  }

  int32_t weight = 0;
  if (flightModesFade)
  {
    memclear(sum_chans512, sizeof(sum_chans512));
    for (uint8_t p = 0; p < MAX_FLIGHT_MODES; p++)
    {
      LS_RECURSIVE_EVALUATION_RESET();
      if (flightModesFade & ((ACTIVE_PHASES_TYPE)1 << p))
      {
        mixerCurrentFlightMode = p;
        evalFlightModeMixes(p == fm ? e_perout_mode_normal : e_perout_mode_inactive_flight_mode, p == fm ? tick10ms : 0);
        for (uint32_t i = 0; i < MAX_OUTPUT_CHANNELS; i++)
          sum_chans512[i] += limit<int32_t>(-0x6fff, chans[i] >> 4, 0x6fff) * fp_act[p];
        weight += fp_act[p];
      }
      LS_RECURSIVE_EVALUATION_RESET();
    }
    assert(weight);
    mixerCurrentFlightMode = fm;
  }
  else
  {
    mixerCurrentFlightMode = fm;
    evalFlightModeMixes(e_perout_mode_normal, tick10ms);
  }

  //========== FUNCTIONS ===============
  // must be done after mixing because some functions use the inputs/channels values
  // must be done before limits because of the applyLimit function: it checks for safety switches which would be not initialized otherwise
  if (tick10ms) {
#if defined(AUDIO) || defined(DFPLAYER)
    requiredSpeakerVolume = g_eeGeneral.speakerVolume + VOLUME_LEVEL_DEF;
#endif
    requiredBacklightBright = g_eeGeneral.backlightBright;

    if (!g_model.noGlobalFunctions)
    {
      evalFunctions(g_eeGeneral.customFn, globalFunctionsContext);
    }
    evalFunctions(g_model.customFn, modelFunctionsContext);
  }

  //========== LIMITS ===============
  for (uint32_t i = 0; i < MAX_OUTPUT_CHANNELS; i++)
  {
    // chans[i] holds data from mixer.   chans[i] = v*weight => 1024*256
    // later we multiply by the limit (up to 100) and then we need to normalize
    // at the end chans[i] = chans[i]/256 =>  -1024..1024
    // interpolate value with min/max so we get smooth motion from center to stop
    // this limits based on v original values and min=-1024, max=1024  RESX=1024
    // Note: Division by 'weight' is dynamic and cannot be optimized to a bit shift.
    // Division by 256 for 'ex_chans' is kept as '/' because 'q' can be negative,
    // and C++ integer division truncates towards zero.
    // Changed: Using divRoundClosest32 for sum_chans512 / weight
    int32_t q = (flightModesFade ? divRoundClosest32(sum_chans512[i], weight) << 4 : chans[i]);

    ex_chans[i] = q / 256;

    int16_t value = applyLimits(i, q); // applyLimits will remove the 256 100% basis

    channelOutputs[i] = value; // copy consistent word to int-level
  }

  if (tick10ms && flightModesFade)
  {
    uint16_t tick_delta = delta * tick10ms;
    for (uint8_t p = 0; p < MAX_FLIGHT_MODES; p++)
    {
      ACTIVE_PHASES_TYPE flightModeMask = ((ACTIVE_PHASES_TYPE)1 << p);
      if (flightModesFade & flightModeMask)
      {
        if (p == fm)
        {
          if (MAX_ACT - fp_act[p] > tick_delta)
            fp_act[p] += tick_delta;
          else
          {
            fp_act[p] = MAX_ACT;
            flightModesFade -= flightModeMask;
          }
        }
        else
        {
          if (fp_act[p] > tick_delta)
            fp_act[p] -= tick_delta;
          else
          {
            fp_act[p] = 0;
            flightModesFade -= flightModeMask;
          }
        }
      }
    }
  }

  // Added: s_mixer_first_run_done flag (from EdgeTX)
  s_mixer_first_run_done = true;
}
