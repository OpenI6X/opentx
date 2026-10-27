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

#ifndef _GTESTS_H_
#define _GTESTS_H_

#include <QtCore/QString>
#include <math.h>
#include <gtest/gtest.h>

#define SWAP_DEFINED
#include "opentx.h"
#include "model_init.h"  // setModelDefaults() moved out of opentx.h

#define CHANNEL_MAX (1024*256)

extern int32_t lastAct;
extern uint16_t anaInValues[NUM_STICKS+NUM_POTS+NUM_SLIDERS];

void doMixerCalculations();

#if defined(PCBTARANIS) || defined(PCBHORUS)
#define RADIO_RESET() \
  g_eeGeneral.switchConfig = 0x00007bff
#else
  #define RADIO_RESET()
#endif

// Switch names in tests: 9X-style (THR, ID0-ID2, ...) or SA-SH style. TR() picks by screen
// width, which no longer tells the layouts apart: PCBI6X is 128 px wide but has SA-SF.
#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBI6X)
  #define TR_SW(x, y)  y
#else
  #define TR_SW(x, y)  x
#endif

inline void SYSTEM_RESET()
{
#if defined(EEPROM)
  memset(modelHeaders, 0, sizeof(modelHeaders));
#endif
  generalDefault();
  g_eeGeneral.templateSetup = 0;
  g_eeGeneral.stickMode = 0;  // tests index sticks physically (mode 1); DEFAULT_MODE targets (I6X) default to mode 2
#if defined(STICK_DEAD_ZONE)
  g_eeGeneral.stickDeadZone = 0;  // like calibration, the I6X stick dead zone is not part of what mixer tests check
#endif
  for (int i=0; i<NUM_SWITCHES; i++) {
    simuSetSwitch(i, -1);
  }
}

inline void MODEL_RESET()
{
  memset(&g_model, 0, sizeof(g_model));
  memset(&anaInValues, 0, sizeof(anaInValues));
  extern uint8_t s_mixer_first_run_done;
  s_mixer_first_run_done = false;
  evalMixes(1);  // this is needed to reset fp_act
  lastFlightMode = 255;
}

inline void MIXER_RESET()
{
  memset(channelOutputs, 0, sizeof(channelOutputs));
  memset(chans, 0, sizeof(chans));
  memset(ex_chans, 0, sizeof(ex_chans));
  memset(act, 0, sizeof(act));
  memset(swOn, 0, sizeof(swOn));
  mixerCurrentFlightMode = lastFlightMode = 0;
  lastAct = 0;
  logicalSwitchesReset();
}

inline void TELEMETRY_RESET()
{
#if defined(TELEMETRY_FRSKY)
  memclear(&telemetryData, sizeof(telemetryData));
  TELEMETRY_RSSI() = 100;
#endif
#if defined(TELEMETRY_FRSKY)
  for (int i=0; i<MAX_TELEMETRY_SENSORS; i++) {
    telemetryItems[i].clear();
  }
#endif
  memclear(g_model.telemetrySensors, sizeof(g_model.telemetrySensors));
}

class OpenTxTest : public testing::Test 
{
  protected:  // You should make the members protected s.t. they can be
              // accessed from sub-classes.

    // virtual void SetUp() will be called before each test is run.  You
    // should define it if you need to initialize the varaibles.
    // Otherwise, this can be skipped.
    virtual void SetUp() 
    {
      SYSTEM_RESET();
      MODEL_RESET();
      MIXER_RESET();
      setModelDefaults();
      RADIO_RESET();
    }
};

#endif // _GTESTS_H_
