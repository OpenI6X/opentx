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

void clearInputs()
{
  memset(g_model.expoData, 0, sizeof(g_model.expoData));
}

void setDefaultInputs() 
{
  for (int i = 0; i < NUM_STICKS; i++) {
    uint8_t stick_index = channelOrder(i + 1);
    ExpoData *expo = expoAddress(i);
    expo->srcRaw = MIXSRC_Rud - 1 + stick_index;
    expo->curve.type = CURVE_REF_EXPO;
    expo->chn = i;
    expo->weight = 100;
    expo->mode = 3;  // TODO constant

#if defined(TRANSLATIONS_CZ)
    for (int c = 0; c < 4; c++) {
      g_model.inputNames[i][c] = char2zchar(STR_INPUTNAMES[1 + 4 * (stick_index - 1) + c]);
    }
#else
    for (int c = 0; c < 3; c++) {
      g_model.inputNames[i][c] = char2zchar(STR_VSRCRAW[2 + 4 * stick_index + c]);
    }
#if LEN_INPUT_NAME > 3
    g_model.inputNames[i][3] = '\0';
#endif
#endif
  }

  storageDirty(EE_MODEL);
}

void setDefaultMixes()
{
  for (int i = 0; i < NUM_STICKS; i++) {
    MixData *mix = mixAddress(i);
    mix->destCh = i;
    mix->weight = 100;
    mix->srcRaw = i + 1;
  }
}

void setDefaultGVars()
{
#if defined(FLIGHT_MODES) && defined(GVARS)
  for (int fmIdx = 1; fmIdx < MAX_FLIGHT_MODES; fmIdx++) {
    for (int gvarIdx = 0; gvarIdx < MAX_GVARS; gvarIdx++) {
      g_model.flightModeData[fmIdx].gvars[gvarIdx] = GVAR_MAX + 1;
    }
  }
#endif
}

void setDefaultRSSIValues()
{
  // https://www.multi-module.org/using-the-module/protocol-details/flysky-afhds2a
  // AFHDS2A suggested OpenTX RSSI alarm threshold settings:
  g_model.rfAlarms.warning = 70;
  g_model.rfAlarms.critical = 50;
}

void setVendorSpecificModelDefaults(uint8_t id)
{
#if defined(PCBI6X)
  // g_model.moduleData[INTERNAL_MODULE].rfProtocol = RF_I6X_PROTO_OFF; // not needed
#endif
}

void applyDefaultTemplate() {
  setDefaultInputs();
  setDefaultMixes();
  setDefaultGVars();
  setDefaultRSSIValues();
}

void setModelDefaults(uint8_t id)
{
  memset(&g_model, 0, sizeof(g_model));
  applyDefaultTemplate();

  setVendorSpecificModelDefaults(id);
}
