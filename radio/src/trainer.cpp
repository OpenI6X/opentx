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

#include "trainer.h"

int16_t trainerInput[MAX_TRAINER_CHANNELS];
uint8_t trainerInputValidityTimer;
uint8_t currentTrainerMode = 0xff;

void checkTrainerSignalWarning()
{
  enum {
    TRAINER_IN_IS_NOT_USED=0,
    TRAINER_IN_IS_VALID,
    TRAINER_IN_INVALID
  };

  static uint8_t trainerInputValidState = TRAINER_IN_IS_NOT_USED;

  if (trainerInputValidityTimer && (trainerInputValidState == TRAINER_IN_IS_NOT_USED)) {
    trainerInputValidState = TRAINER_IN_IS_VALID;
  }
  else if (!trainerInputValidityTimer && (trainerInputValidState == TRAINER_IN_IS_VALID)) {
    trainerInputValidState = TRAINER_IN_INVALID;
    AUDIO_TRAINER_LOST();
  }
  else if (trainerInputValidityTimer && (trainerInputValidState == TRAINER_IN_INVALID)) {
    trainerInputValidState = TRAINER_IN_IS_VALID;
    AUDIO_TRAINER_BACK();
  }
}

void checkTrainerSettings()
{
  uint8_t requiredTrainerMode = g_model.trainerMode;

  if (requiredTrainerMode != currentTrainerMode) {
    switch (currentTrainerMode) {
      case TRAINER_MODE_MASTER_TRAINER_JACK:
        stop_trainer_capture();
        break;
#if !defined(PCBI6X)
      case TRAINER_MODE_SLAVE:
        stop_trainer_ppm();
        break;
      case TRAINER_MODE_MASTER_CPPM_EXTERNAL_MODULE:
        stop_cppm_on_heartbeat_capture() ;
        break;
      case TRAINER_MODE_MASTER_SBUS_EXTERNAL_MODULE:
        stop_sbus_on_heartbeat_capture() ;
        break;
#endif
#if defined(TRAINER_BATTERY_COMPARTMENT)
      case TRAINER_MODE_MASTER_BATTERY_COMPARTMENT:
        auxSerialStop();
        break;
#endif
    }

    currentTrainerMode = requiredTrainerMode;

    switch (requiredTrainerMode) {
#if !defined(PCBI6X)
    case TRAINER_MODE_SLAVE:
      init_trainer_ppm();
      break;
    case TRAINER_MODE_MASTER_CPPM_EXTERNAL_MODULE:
        init_cppm_on_heartbeat_capture();
        break;
    case TRAINER_MODE_MASTER_SBUS_EXTERNAL_MODULE:
        init_sbus_on_heartbeat_capture();
        break;
#endif
#if defined(TRAINER_BATTERY_COMPARTMENT)
    case TRAINER_MODE_MASTER_BATTERY_COMPARTMENT:
#if defined(AUX_SERIAL)
      if (g_eeGeneral.auxSerialMode == UART_MODE_SBUS_TRAINER) {
        auxSerialSbusInit();
        break;
      }
#endif
      // no break
#endif
    default:
      // master is default
      init_trainer_capture();
      break;
    }

#if defined(TRAINER_MODULE_CPPM) || defined(TRAINER_MODULE_SBUS)
    if (requiredTrainerMode == TRAINER_MODE_MASTER_CPPM_EXTERNAL_MODULE || requiredTrainerMode == TRAINER_MODE_MASTER_SBUS_EXTERNAL_MODULE)
      stop_intmodule_heartbeat();
    else
      init_intmodule_heartbeat();
#else
    // init_intmodule_heartbeat();
#endif
  }
}

