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

#if defined(PXX2)
#include "io/pxx2.h"
#include "pulses/pxx2.h"
#endif
#include "mixer_scheduler.h"

uint8_t s_pulses_paused = 0;
ModuleState moduleState[NUM_MODULES];
InternalModulePulsesData intmodulePulsesData __DMA;
ExternalModulePulsesData extmodulePulsesData __DMA;
TrainerPulsesData trainerPulsesData __DMA;

#if defined(CROSSFIRE)
uint8_t createCrossfireChannelsFrame(uint8_t* frame, int16_t* pulses);
#endif

uint8_t getRequiredProtocol(uint8_t module) 
{
  uint8_t protocol;

  switch (module) {
#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBI6X)
    case INTERNAL_MODULE:
      switch (g_model.moduleData[INTERNAL_MODULE].type) {
        case MODULE_TYPE_AFHDS2A_SPI:
          protocol = PROTOCOL_CHANNELS_AFHDS2A_SPI;
          break;
        default:
          protocol = PROTOCOL_CHANNELS_NONE;
          break;
      }
      break;
#endif

    default:
      switch (g_model.moduleData[EXTERNAL_MODULE].type) {
        case MODULE_TYPE_PPM:
          protocol = PROTOCOL_CHANNELS_PPM;
          break;
#if !defined(PCBI6X)
        case MODULE_TYPE_SBUS:
          protocol = PROTOCOL_CHANNELS_SBUS;
          break;
#endif
#if defined(MULTIMODULE)
        case MODULE_TYPE_MULTIMODULE:
          protocol = PROTOCOL_CHANNELS_MULTIMODULE;
          break;
#endif
#if defined(CROSSFIRE)
        case MODULE_TYPE_CROSSFIRE:
          protocol = PROTOCOL_CHANNELS_CROSSFIRE;
          break;
#endif
        default:
          protocol = PROTOCOL_CHANNELS_NONE;
          break;
      }
      break;
  }

  if (s_pulses_paused) {
    protocol = PROTOCOL_CHANNELS_NONE;
  }

#if 0
  // will need an EEPROM conversion
  if (moduleState[module].mode == MODULE_OFF) {
    protocol = PROTOCOL_CHANNELS_NONE;
  }
#endif

  return protocol;
}

#if defined(HARDWARE_EXTERNAL_MODULE)
void enablePulsesExternalModule(uint8_t protocol)
{
  // start new protocol hardware here

  switch (protocol) {
#if defined(CROSSFIRE)
    case PROTOCOL_CHANNELS_CROSSFIRE:
      EXTERNAL_MODULE_ON();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, CROSSFIRE_PERIOD);
      break;
#endif

#if defined(MULTIMODULE)
    case PROTOCOL_CHANNELS_MULTIMODULE:
      extmoduleSerialStart();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, MULTIMODULE_PERIOD);
      break;
#endif

#if defined(SBUS) && !defined(PCBI6X)
    case PROTOCOL_CHANNELS_SBUS:
      extmoduleSerialStart();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, SBUS_PERIOD);
      break;
#endif

#if defined(PPM)
    case PROTOCOL_CHANNELS_PPM:
      extmodulePpmStart();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, PPM_PERIOD(EXTERNAL_MODULE));
      break;
#endif

    default:
      // external module stopped, use default mixer period
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, 0);
      break;
  }
}

bool setupPulsesExternalModule(uint8_t protocol)
{
  switch (protocol) {
#if defined(SBUS) && !defined(PCBI6X)
    case PROTOCOL_CHANNELS_SBUS:
      setupPulsesSbus();
      // SBUS_PERIOD is not a constant! It can be set from UI
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, SBUS_PERIOD);
      return true;
#endif

#if defined(CROSSFIRE)
    case PROTOCOL_CHANNELS_CROSSFIRE:
    {
      ModuleSyncStatus& status = getModuleSyncStatus(EXTERNAL_MODULE);
      if (status.isValid()) {
        mixerSchedulerSetPeriod(EXTERNAL_MODULE, status.getAdjustedRefreshRate());
      }
      else
        mixerSchedulerSetPeriod(EXTERNAL_MODULE, CROSSFIRE_PERIOD);
      setupPulsesCrossfire();
      return true;
    }
#endif

#if defined(MULTIMODULE)
    case PROTOCOL_CHANNELS_MULTIMODULE:
    {
      ModuleSyncStatus& status = getModuleSyncStatus(EXTERNAL_MODULE);
      if (status.isValid())
        mixerSchedulerSetPeriod(EXTERNAL_MODULE, status.getAdjustedRefreshRate());
      else
        mixerSchedulerSetPeriod(EXTERNAL_MODULE, MULTIMODULE_PERIOD);
      setupPulsesMultiExternalModule();
      return true;
    }
#endif

#if defined(PPM)
    case PROTOCOL_CHANNELS_PPM:
      setupPulsesPPMExternalModule();
      return true;
#endif

    default:
      return false;
  }
}
#endif

#if defined(HARDWARE_INTERNAL_MODULE)
static void enablePulsesInternalModule(uint8_t protocol)
{
  // start new protocol hardware here

  switch (protocol) {
#if defined(AFHDS2A)
    case PROTOCOL_CHANNELS_AFHDS2A_SPI:
      intmoduleAfhds2aStart();
      mixerSchedulerSetPeriod(INTERNAL_MODULE, AFHDS2A_PERIOD);
    break;
#endif

    default:
      // internal module stopped, use default mixer period
      mixerSchedulerSetPeriod(INTERNAL_MODULE, 0);
      break;
  }
}

bool setupPulsesInternalModule(uint8_t protocol)
{
  switch (protocol) {
    case PROTOCOL_CHANNELS_AFHDS2A_SPI:
      // nothing needed, AFHDS2A does setup and sends frame in ActionAFHDS2A
      return true;

    default:
      //mixerSchedulerSetPeriod(INTERNAL_MODULE, 10000 /*us*/); // used for USB sim for example
      return false;
  }
}

void stopPulsesInternalModule()
{
  if (moduleState[INTERNAL_MODULE].protocol != PROTOCOL_CHANNELS_UNINITIALIZED) {
    mixerSchedulerSetPeriod(INTERNAL_MODULE, 0);
    intmoduleStop();
    moduleState[INTERNAL_MODULE].protocol = PROTOCOL_CHANNELS_NONE;
  }
}

bool setupPulsesInternalModule()
{
  uint8_t protocol = getRequiredProtocol(INTERNAL_MODULE);

  heartbeat |= (HEART_TIMER_PULSES << INTERNAL_MODULE);

  if (moduleState[INTERNAL_MODULE].protocol != protocol) {
    intmoduleStop();
    moduleState[INTERNAL_MODULE].protocol = protocol;
    enablePulsesInternalModule(protocol);
    return false;
  }
  else {
    return setupPulsesInternalModule(protocol);
  }
}
#endif

#if defined(HARDWARE_EXTERNAL_MODULE)
void stopPulsesExternalModule()
{
  if (moduleState[EXTERNAL_MODULE].protocol != PROTOCOL_CHANNELS_UNINITIALIZED) {
    mixerSchedulerSetPeriod(EXTERNAL_MODULE, 0);
    extmoduleStop();
    moduleState[EXTERNAL_MODULE].protocol = PROTOCOL_CHANNELS_NONE;
  }
}

bool setupPulsesExternalModule()
{
  uint8_t protocol = getRequiredProtocol(EXTERNAL_MODULE);

  heartbeat |= (HEART_TIMER_PULSES << EXTERNAL_MODULE);

  if (moduleState[EXTERNAL_MODULE].protocol != protocol) {
    extmoduleStop();
    moduleState[EXTERNAL_MODULE].protocol = protocol;
    enablePulsesExternalModule(protocol);
    setupPulsesExternalModule(protocol);
    return false;
  }
  else {
    return setupPulsesExternalModule(protocol);
  }
}
#endif

void setCustomFailsafe(uint8_t moduleIndex) {
  if (moduleIndex < NUM_MODULES) {
    for (int ch = 0; ch < MAX_OUTPUT_CHANNELS; ch++) {
      if (ch < g_model.moduleData[moduleIndex].channelsStart || ch >= sentModuleChannels(moduleIndex) + g_model.moduleData[moduleIndex].channelsStart) {
        g_model.failsafeChannels[ch] = 0;
      } else if (g_model.failsafeChannels[ch] < FAILSAFE_CHANNEL_HOLD) {
        g_model.failsafeChannels[ch] = channelOutputs[ch];
      }
    }
  }
}
