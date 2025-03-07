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

#include "MultiSubtypeDefs.h"

int circularIncDec(int current, int inc, int min, int max, IsValueAvailable isValueAvailable) {
  do {
    current += inc;
    if (current < min)
      current = max;
    else if (current > max)
      current = min;
    if (!isValueAvailable || isValueAvailable(current))
      return current;
  } while (1);
  return 0;
}

bool isInputAvailable(int input) {
  for (int i = 0; i < MAX_EXPOS; i++) {
    ExpoData *expo = expoAddress(i);
    if (!EXPO_VALID(expo))
      break;
    if (expo->chn == input)
      return true;
  }
  return false;
}

bool isSensorAvailable(int sensor) {
  if (sensor == 0)
    return true;
  else
    return isTelemetryFieldAvailable(abs(sensor) - 1);
}

bool isSensorUnit(int sensor, uint8_t unit) {
  if (sensor <= 0 || sensor > MAX_TELEMETRY_SENSORS) {
    return true;
  } else {
    return g_model.telemetrySensors[sensor - 1].unit == unit;
  }
}

bool isCellsSensor(int sensor) {
  return isSensorUnit(sensor, UNIT_CELLS);
}

bool isGPSSensor(int sensor) {
  return isSensorUnit(sensor, UNIT_GPS);
}

bool isAltSensor(int sensor) {
  return isSensorUnit(sensor, UNIT_DIST) || isSensorUnit(sensor, UNIT_FEET);
}

bool isVoltsSensor(int sensor) {
  return isSensorUnit(sensor, UNIT_VOLTS) || isSensorUnit(sensor, UNIT_CELLS);
}

bool isCurrentSensor(int sensor) {
  return isSensorUnit(sensor, UNIT_AMPS);
}

bool isTelemetryFieldAvailable(int index) {
  if (index>=MAX_TELEMETRY_SENSORS){
    return false;
  }
  TelemetrySensor &sensor = g_model.telemetrySensors[index];
  return sensor.isAvailable();
}

bool isTelemetryFieldComparisonAvailable(int index) {
  if (!isTelemetryFieldAvailable(index))
    return false;

  TelemetrySensor &sensor = g_model.telemetrySensors[index];
  if (sensor.unit >= UNIT_DATETIME)
    return false;
  return true;
}

bool isChannelUsed(int channel) {
  for (int i = 0; i < MAX_MIXERS; ++i) {
    MixData *md = mixAddress(i);
    if (md->srcRaw == 0) return false;
    if (md->destCh == channel) return true;
    if (md->destCh > channel) return false;
  }
  return false;
}

int getChannelsUsed() {
  int result = 0;
  int lastCh = -1;
  for (int i = 0; i < MAX_MIXERS; ++i) {
    MixData *md = mixAddress(i);
    if (md->srcRaw == 0) return result;
    if (md->destCh != lastCh) {
      ++result;
      lastCh = md->destCh;
    }
  }
  return result;
}

bool isSourceAvailable(int source) {
  if (source < 0)
    return false;

  if (source >= MIXSRC_FIRST_INPUT && source <= MIXSRC_LAST_INPUT) {
    return isInputAvailable(source - MIXSRC_FIRST_INPUT);
  }

#if defined(LUA_MODEL_SCRIPTS)
  if (source >= MIXSRC_FIRST_LUA && source <= MIXSRC_LAST_LUA) {
    div_t qr = div(source - MIXSRC_FIRST_LUA, MAX_SCRIPT_OUTPUTS);
    return (qr.rem < scriptInputsOutputs[qr.quot].outputsCount);
  }
#elif defined(LUA_INPUTS)
  if (source >= MIXSRC_FIRST_LUA && source <= MIXSRC_LAST_LUA)
    return false;
#endif

  if (source >= MIXSRC_FIRST_POT && source <= MIXSRC_LAST_POT) {
    return IS_POT_SLIDER_AVAILABLE(POT1 + source - MIXSRC_FIRST_POT);
  }

#if defined(PCBX10)
  if ((source >= MIXSRC_S3 && source <= MIXSRC_S4) || (source >= MIXSRC_MOUSE1 && source <= MIXSRC_MOUSE2))
    return false;
#endif

  if (source >= MIXSRC_FIRST_SWITCH && source <= MIXSRC_LAST_SWITCH) {
    return SWITCH_EXISTS(source - MIXSRC_FIRST_SWITCH);
  }

#if !defined(HELI)
  if (source >= MIXSRC_CYC1 && source <= MIXSRC_CYC3)
    return false;
#endif

  if (source >= MIXSRC_FIRST_CH && source <= MIXSRC_LAST_CH) {
    return isChannelUsed(source - MIXSRC_FIRST_CH);
  }

  if (source >= MIXSRC_FIRST_LOGICAL_SWITCH && source <= MIXSRC_LAST_LOGICAL_SWITCH) {
    LogicalSwitchData *cs = lswAddress(source - MIXSRC_FIRST_LOGICAL_SWITCH);
    return (cs->func != LS_FUNC_NONE);
  }

#if !defined(GVARS)
  if (source >= MIXSRC_GVAR1 && source <= MIXSRC_LAST_GVAR)
    return false;
#endif

  if (source >= MIXSRC_FIRST_RESERVE && source <= MIXSRC_LAST_RESERVE)
    return false;

  if (source >= MIXSRC_FIRST_TELEM && source <= MIXSRC_LAST_TELEM) {
    div_t qr = div(source - MIXSRC_FIRST_TELEM, 3);
    if (qr.rem == 0)
      return isTelemetryFieldAvailable(qr.quot);
    else
      return isTelemetryFieldComparisonAvailable(qr.quot);
  }

  return true;
}

bool isSourceAvailableInGlobalFunctions(int source) {
  if (source >= MIXSRC_FIRST_TELEM && source <= MIXSRC_LAST_TELEM) {
    return false;
  }
  return isSourceAvailable(source);
}

bool isSourceAvailableInCustomSwitches(int source) {
  bool result = isSourceAvailable(source);

#if defined(TELEMETRY_FRSKY)
  if (result && source >= MIXSRC_FIRST_TELEM && source <= MIXSRC_LAST_TELEM) {
    div_t qr = div(source - MIXSRC_FIRST_TELEM, 3);
    result = isTelemetryFieldComparisonAvailable(qr.quot);
  }
#endif

  return result;
}

bool isSourceAvailableInInputs(int source) {
  if (source >= MIXSRC_FIRST_POT && source <= MIXSRC_LAST_POT) {
    return IS_POT_SLIDER_AVAILABLE(POT1 + source - MIXSRC_FIRST_POT);
  }

  if (source >= MIXSRC_Rud && source <= MIXSRC_MAX)
    return true;

  if (source >= MIXSRC_FIRST_TRIM && source <= MIXSRC_LAST_TRIM)
    return true;

  if (source >= MIXSRC_FIRST_SWITCH && source <= MIXSRC_LAST_SWITCH)
    return SWITCH_EXISTS(source - MIXSRC_FIRST_SWITCH);

  if (source >= MIXSRC_FIRST_CH && source <= MIXSRC_LAST_CH)
    return true;

  if (source >= MIXSRC_FIRST_LOGICAL_SWITCH && source <= MIXSRC_LAST_LOGICAL_SWITCH) {
    LogicalSwitchData *cs = lswAddress(source - MIXSRC_SW1);
    return (cs->func != LS_FUNC_NONE);
  }

  if (source >= MIXSRC_FIRST_TRAINER && source <= MIXSRC_LAST_TRAINER)
    return true;

  if (source >= MIXSRC_FIRST_TELEM && source <= MIXSRC_LAST_TELEM) {
    div_t qr = div(source - MIXSRC_FIRST_TELEM, 3);
    return isTelemetryFieldAvailable(qr.quot) && isTelemetryFieldComparisonAvailable(qr.quot);
  }

  return false;
}

enum SwitchContext {
  LogicalSwitchesContext,
  ModelCustomFunctionsContext,
  GeneralCustomFunctionsContext,
  TimersContext,
  MixesContext
};

bool isLogicalSwitchAvailable(int index) {
  LogicalSwitchData *lsw = lswAddress(index);
  return (lsw->func != LS_FUNC_NONE);
}

bool isSwitchAvailable(int swtch, SwitchContext context) {
  bool negative = false;

  if (swtch < 0) {
    if (swtch == -SWSRC_ON || swtch == -SWSRC_ONE) {
      return false;
    }
    negative = true;
    swtch = -swtch;
  }

#if defined(PCBI6X)
  if (swtch >= SWSRC_FIRST_SWITCH && swtch <= SWSRC_LAST_SWITCH) {
    UNUSED(negative);
    return true;
  }
#else
  if (swtch >= SWSRC_FIRST_SWITCH && swtch <= SWSRC_LAST_SWITCH) {
    div_t swinfo = switchInfo(swtch);
    if (!SWITCH_EXISTS(swinfo.quot)) {
      return false;
    }
    if (!IS_CONFIG_3POS(swinfo.quot)) {
      if (negative) {
        return false;
      }
      if (swinfo.rem == 1) {
        // mid position not available for 2POS switches
        return false;
      }
    }
    return true;
  }
#endif

#if NUM_XPOTS > 0
  if (swtch >= SWSRC_FIRST_MULTIPOS_SWITCH && swtch <= SWSRC_LAST_MULTIPOS_SWITCH) {
    int index = (swtch - SWSRC_FIRST_MULTIPOS_SWITCH) / XPOTS_MULTIPOS_COUNT;
    if (IS_POT_MULTIPOS(POT1 + index)) {
      StepsCalibData *calib = (StepsCalibData *)&g_eeGeneral.calib[POT1 + index];
      return (calib->count >= ((swtch - SWSRC_FIRST_MULTIPOS_SWITCH) % XPOTS_MULTIPOS_COUNT));
    } else {
      return false;
    }
  }
#endif

  if (swtch >= SWSRC_FIRST_LOGICAL_SWITCH && swtch <= SWSRC_LAST_LOGICAL_SWITCH) {
    if (context == GeneralCustomFunctionsContext) {
      return false;
    } else if (context != LogicalSwitchesContext) {
      return isLogicalSwitchAvailable(swtch - SWSRC_FIRST_LOGICAL_SWITCH);
    }
  }

  if (context != ModelCustomFunctionsContext && context != GeneralCustomFunctionsContext && (swtch == SWSRC_ON || swtch == SWSRC_ONE)) {
    return false;
  }

  if (swtch >= SWSRC_FIRST_FLIGHT_MODE && swtch <= SWSRC_LAST_FLIGHT_MODE) {
    if (context == MixesContext || context == GeneralCustomFunctionsContext) {
      return false;
    } else {
      swtch -= SWSRC_FIRST_FLIGHT_MODE;
      if (swtch == 0) {
        return true;
      }
      FlightModeData *fm = flightModeAddress(swtch);
      return (fm->swtch != SWSRC_NONE);
    }
  }

  if (swtch >= SWSRC_FIRST_SENSOR && swtch <= SWSRC_LAST_SENSOR) {
    if (context == GeneralCustomFunctionsContext)
      return false;
    else
      return isTelemetryFieldAvailable(swtch - SWSRC_FIRST_SENSOR);
  }

  return true;
}

bool isSwitchAvailableInLogicalSwitches(int swtch) {
  return isSwitchAvailable(swtch, LogicalSwitchesContext);
}

bool isSwitchAvailableInCustomFunctions(int swtch) {
  if (menuHandlers[menuLevel] == menuModelSpecialFunctions)
    return isSwitchAvailable(swtch, ModelCustomFunctionsContext);
  else
    return isSwitchAvailable(swtch, GeneralCustomFunctionsContext);
}

bool isSwitchAvailableInMixes(int swtch) {
  return isSwitchAvailable(swtch, MixesContext);
}

bool isSwitchAvailableForArming(int swtch)
{
  return isSwitchAvailable(swtch, ModelCustomFunctionsContext);
}

#if defined(COLORLCD)
bool isSwitch2POSWarningStateAvailable(int state) {
  return (state != 2);  // two pos switch - middle state not available
}
#endif  // #if defined(COLORLCD)

bool isSwitchAvailableInTimers(int swtch) {
  if (swtch >= 0) {
    if (swtch < TMRMODE_COUNT)
      return true;
    else
      swtch -= TMRMODE_COUNT - 1;
  } else {
    if (swtch > -TMRMODE_COUNT)
      return false;
    else
      swtch += TMRMODE_COUNT - 1;
  }

  return isSwitchAvailable(swtch, TimersContext);
}

bool isThrottleSourceAvailable(int source) {
  if (source >= THROTTLE_SOURCE_FIRST_POT && source < THROTTLE_SOURCE_FIRST_POT + NUM_POTS + NUM_SLIDERS && !IS_POT_SLIDER_AVAILABLE(POT1 + source - THROTTLE_SOURCE_FIRST_POT))
    return false;
  else
    return true;
}

bool isLogicalSwitchFunctionAvailable(int function) {
  return function != LS_FUNC_RANGE;
}

bool isAssignableFunctionAvailable(int function) {
#if defined(OVERRIDE_CHANNEL_FUNCTION) || defined(GVARS)
  bool modelFunctions = (menuHandlers[menuLevel] == menuModelSpecialFunctions);
#endif

  switch (function) {
    case FUNC_OVERRIDE_CHANNEL:
#if defined(OVERRIDE_CHANNEL_FUNCTION)
      return modelFunctions;
#else
      return false;
#endif
    case FUNC_ADJUST_GVAR:
#if defined(GVARS)
      return modelFunctions;
#else
      return false;
#endif
#if defined(PCBI6X) // volume function unsupported
    case FUNC_VOLUME:
#endif
#if !defined(HAPTIC)
    case FUNC_HAPTIC:
#endif
    case FUNC_RESERVE4:
#if !defined(DANGEROUS_MODULE_FUNCTIONS)
    case FUNC_RANGECHECK:
    case FUNC_BIND:
#endif
#if !defined(LUA)
    case FUNC_PLAY_SCRIPT:
#endif
    case FUNC_RESERVE5:
      return false;

    default:
      return true;
  }
}

bool isSourceAvailableInGlobalResetSpecialFunction(int index) {
  if (index >= FUNC_RESET_PARAM_FIRST_TELEM)
    return false;
  else
    return isSourceAvailableInResetSpecialFunction(index);
}

bool isSourceAvailableInResetSpecialFunction(int index) {
  if (index >= FUNC_RESET_PARAM_FIRST_TELEM) {
    TelemetrySensor &telemetrySensor = g_model.telemetrySensors[index - FUNC_RESET_PARAM_FIRST_TELEM];
    return telemetrySensor.isAvailable();
  }
#if TIMERS < 3
  else if (index == FUNC_RESET_TIMER3) {
    return false;
  }
#endif
#if TIMERS < 2
  else if (index == FUNC_RESET_TIMER2) {
    return false;
  }
#endif
  else {
    return true;
  }
}

bool isR9MModeAvailable(int mode) {
#if defined(MODULE_R9M_FLEX_FW)
  return mode >= MODULE_SUBTYPE_R9M_EUPLUS;
#else
  return mode <= MODULE_SUBTYPE_R9M_EU;
#endif
}

#if defined(PCBI6X)
bool isSubtypeAvailable(int i){
  return true;
}
#endif

bool isModuleAvailable(int module) {
#if defined(CROSSFIRE)
  if (module == MODULE_TYPE_CROSSFIRE && g_model.moduleData[INTERNAL_MODULE].type != MODULE_TYPE_NONE) {
    return false;
  }
#else
  if (module == MODULE_TYPE_CROSSFIRE) {
    return false;
  }
#endif
#if !defined(PCBI6X)
  #if !defined(DSM2)
    if (module == MODULE_TYPE_DSM2) {
      return false;
    }
  #endif
  #if !defined(MULTIMODULE)
    if (module == MODULE_TYPE_MULTIMODULE) {
      return false;
    }
  #endif
#endif
  return true;
}

bool isRfProtocolAvailable(int protocol) {
#if defined(CROSSFIRE)
  if (protocol != RF_PROTO_OFF && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_CROSSFIRE) {
    return false;
  }
#endif
#if defined(MODULE_D16_EU_ONLY_SUPPORT)
  if (protocol == RF_PROTO_D8) {
    return false;
  }
#endif
  return true;
}

bool isTelemetryProtocolAvailable(int protocol) {
#if defined(PCBTARANIS)
  if (protocol == PROTOCOL_TELEMETRY_FRSKY_D_SECONDARY && g_eeGeneral.auxSerialMode != UART_MODE_TELEMETRY) {
    return false;
  }
#endif

  if (protocol == PROTOCOL_TELEMETRY_CROSSFIRE) {
    return false;
  }
#if defined(PCBI6X)
  if (protocol == PROTOCOL_TELEMETRY_FLYSKY_IBUS) {
    return true;
  }
#endif
#if !defined(MULTIMODULE) && !defined(PCBI6X)
  if (protocol == PROTOCOL_TELEMETRY_SPEKTRUM || protocol == PROTOCOL_TELEMETRY_FLYSKY_IBUS || protocol == PROTOCOL_TELEMETRY_MULTIMODULE) {
    return false;
  }
#endif

#if defined(PCBHORUS)
  if (protocol == PROTOCOL_TELEMETRY_FRSKY_D_SECONDARY) {
    return false;
  }
#endif

  return true;
}

#if defined(PCBHORUS) || defined(PCBI6X)
bool isTrainerModeAvailable(int mode) {
  return true;
}
#elif defined(PCBX9E)
bool isTrainerModeAvailable(int mode) {
  if (IS_EXTERNAL_MODULE_ENABLED() && (mode == TRAINER_MODE_MASTER_SBUS_EXTERNAL_MODULE || mode == TRAINER_MODE_MASTER_CPPM_EXTERNAL_MODULE))
    return false;
#if defined(USEHORUSBT)
  else if (mode == TRAINER_MODE_MASTER_BATTERY_COMPARTMENT)
#else
  else if (mode == TRAINER_MODE_MASTER_BLUETOOTH || mode == TRAINER_MODE_MASTER_BATTERY_COMPARTMENT || mode == TRAINER_MODE_SLAVE_BLUETOOTH)
#endif
    return false;
  else
    return true;
}
#elif defined(PCBX9)
bool isTrainerModeAvailable(int mode) {
  if (IS_EXTERNAL_MODULE_ENABLED() && (mode == TRAINER_MODE_MASTER_SBUS_EXTERNAL_MODULE || mode == TRAINER_MODE_MASTER_CPPM_EXTERNAL_MODULE))
    return false;
  else
    return true;
}
#elif defined(PCBX7)
bool isTrainerModeAvailable(int mode) {
  if (IS_EXTERNAL_MODULE_ENABLED() && (mode == TRAINER_MODE_MASTER_SBUS_EXTERNAL_MODULE || mode == TRAINER_MODE_MASTER_CPPM_EXTERNAL_MODULE))
    return false;
  else if (mode == TRAINER_MODE_MASTER_BATTERY_COMPARTMENT)
    return false;
#if defined(BLUETOOTH)
  else if (g_eeGeneral.bluetoothMode != BLUETOOTH_TRAINER && (mode == TRAINER_MODE_MASTER_BLUETOOTH || mode == TRAINER_MODE_SLAVE_BLUETOOTH))
    return false;
#endif
  else
    return true;
}
#elif defined(PCBXLITE)
bool isTrainerModeAvailable(int mode) {
  if (g_eeGeneral.bluetoothMode == BLUETOOTH_TRAINER && (mode == TRAINER_MODE_MASTER_BLUETOOTH || mode == TRAINER_MODE_SLAVE_BLUETOOTH))
    return true;
  else
    return false;
}
#endif

bool modelHasNotes() {
#if defined(SDCARD)
  char filename[sizeof(MODELS_PATH) + 1 + sizeof(g_model.header.name) + sizeof(TEXT_EXT)] = MODELS_PATH "/";
  char *buf = strcat_currentmodelname(&filename[sizeof(MODELS_PATH)]);
  strcpy(buf, TEXT_EXT);
  if (isFileAvailable(filename)) {
    return true;
  }
#endif
#if !defined(EEPROM)
  buf = strAppendFilename(&filename[sizeof(MODELS_PATH)], g_eeGeneral.currModelFilename, LEN_MODEL_FILENAME);
  strcpy(buf, TEXT_EXT);
  if (isFileAvailable(filename)) {
    return true;
  }
#endif

  return false;
}

int getFirstAvailable(int min, int max, IsValueAvailable isValueAvailable) {
  int retval = 0;
  for (int i = min; i <= max; i++) {
    if (isValueAvailable(i)) {
      retval = i;
      break;
    }
  }
  return retval;
}

#if defined(MULTIMODULE)

const uint8_t getMaxMultiOptions()
{
  return DIM(mm_options_strings::options);
}

const mm_protocol_definition *getMultiProtocolDefinition (uint8_t protocol)
{
  const mm_protocol_definition *pdef;
  for (pdef = multi_protocols; pdef->protocol != 0xfe; pdef++) {
    if (pdef->protocol == protocol)
      return pdef;
  }
  // Return the empty last protocol
  return pdef;
}

const char * getMultiOptionTitleStatic(uint8_t moduleIdx)
{
  const uint8_t multi_proto = g_model.moduleData[moduleIdx].multi.rfProtocol;
  const mm_protocol_definition * pdef = getMultiProtocolDefinition(multi_proto);
  return pdef->optionsstr;
}

const char * getMultiOptionTitle(uint8_t moduleIdx)
{
  MultiModuleStatus &status = getMultiModuleStatus(moduleIdx);

  if (status.isValid()) {
    if (status.optionDisp >= getMaxMultiOptions()) {
      status.optionDisp = 1; // Unknown options are defaulted to type 1 (basic option)
    }
    return mm_options_strings::options[status.optionDisp];
  }

  return getMultiOptionTitleStatic(moduleIdx);
}
#endif

void editStickHardwareSettings(coord_t x, coord_t y, int idx, event_t event, LcdFlags flags) {
  lcdDrawTextAtIndex(INDENT_WIDTH, y, STR_VSRCRAW, idx + 1, 0);
  if (ZEXIST(g_eeGeneral.anaNames[idx]) || (flags && s_editMode > 0))
    editName(x, y, g_eeGeneral.anaNames[idx], LEN_ANA_NAME, event, flags);
  else
    lcdDrawMMM(x, y, flags);
}
