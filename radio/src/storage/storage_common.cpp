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

uint8_t storageDirtyMsk;
tmr10ms_t storageDirtyTime10ms;

#if defined(RAMBACKUP)
uint8_t rambackupDirtyMsk;
tmr10ms_t rambackupDirtyTime10ms;
#endif

void storageDirty(uint8_t msk) {
  storageDirtyMsk |= msk;
  storageDirtyTime10ms = get_tmr10ms();

#if defined(RAMBACKUP)
  rambackupDirtyMsk = storageDirtyMsk;
  rambackupDirtyTime10ms = storageDirtyTime10ms;
#endif
}

void preModelLoad()
{
  watchdogSuspend(500 /*5s*/);

#if defined(SDCARD)
  logsClose();
#endif

  if (pulsesStarted()) {
    pausePulses();
  }

  pauseMixerCalculations();

#if defined(HARDWARE_INTERNAL_MODULE)
  stopPulsesInternalModule();
#endif
#if defined(HARDWARE_EXTERNAL_MODULE)
  stopPulsesExternalModule();
#endif
//#if defined(TRAINER_GPIO)
//   stopTrainer();
//#endif
}

void postModelLoad(bool alarms)
{
  AUDIO_FLUSH();
  flightReset(false);

  customFunctionsReset();

  restoreTimers();

  for (int i = 0; i < MAX_TELEMETRY_SENSORS; i++) {
    TelemetrySensor& sensor = g_model.telemetrySensors[i];
    if (sensor.type == TELEM_TYPE_CALCULATED && sensor.persistent) {
      telemetryItems[i].value = sensor.persistentValue;
      telemetryItems[i].timeout = 0;  // #3595: make value visible even before the first new value is received)
    } else {
      telemetryItems[i].timeout = TELEMETRY_SENSOR_TIMEOUT_UNAVAILABLE;
    }
  }

  LOAD_MODEL_CURVES();

  resumeMixerCalculations();
  if (pulsesStarted()) {
#if defined(GUI)
    if (alarms) {
      checkAll();
      PLAY_MODEL_NAME();
    }
#endif
    resumePulses();
  }

#if defined(SDCARD)
  referenceModelAudioFiles();
#endif

#if defined(PCBHORUS)
  loadCustomScreens();
#endif

  LOAD_MODEL_BITMAP();
  LUA_LOAD_MODEL_SCRIPTS();
  SEND_FAILSAFE_1S();
}

void storageFlushCurrentModel()
{
  saveTimers();

  for (int i = 0; i < MAX_TELEMETRY_SENSORS; i++) {
    TelemetrySensor& sensor = g_model.telemetrySensors[i];
    if (sensor.type == TELEM_TYPE_CALCULATED && sensor.persistent && sensor.persistentValue != telemetryItems[i].value) {
      sensor.persistentValue = telemetryItems[i].value;
      storageDirty(EE_MODEL);
    }
  }
#if !defined(PCBI6X)
  if (g_model.potsWarnMode == POTS_WARN_AUTO) {
    for (int i = 0; i < NUM_POTS + NUM_SLIDERS; i++) {
      if (!(g_model.potsWarnEnabled & (1 << i))) {
        SAVE_POT_POSITION(i);
      }
    }
    storageDirty(EE_MODEL);
  }
#endif
}
