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

uint8_t getRequiredProtocol(uint8_t port) {
  uint8_t required_protocol;

  switch (port) {
#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBI6X)
    case INTERNAL_MODULE:
      switch (g_model.moduleData[INTERNAL_MODULE].type) {
#if defined(TARANIS_INTERNAL_PPM)
        case MODULE_TYPE_PPM:
          required_protocol = PROTOCOL_CHANNELS_PPM;
          break;
#endif
#if defined(PXX)
        case MODULE_TYPE_XJT:
          required_protocol = PROTO_PXX;
          break;
#endif
        case MODULE_TYPE_AFHDS2A_SPI:
          required_protocol = PROTOCOL_CHANNELS_AFHDS2A_SPI;
          break;
        default:
          required_protocol = PROTOCOL_CHANNELS_NONE;
          break;
      }
      break;
#endif

    default:
      switch (g_model.moduleData[EXTERNAL_MODULE].type) {
        case MODULE_TYPE_PPM:
          required_protocol = PROTOCOL_CHANNELS_PPM;
          break;
#if defined(PXX) || defined(PXX2)
        case MODULE_TYPE_XJT:
        case MODULE_TYPE_R9M:
          required_protocol = PROTO_PXX_EXTERNAL_MODULE;  // either PXX or PXX2 depending on compilation options
          break;
#endif
#if !defined(PCBI6X)
        case MODULE_TYPE_SBUS:
          required_protocol = PROTOCOL_CHANNELS_SBUS;
          break;
#endif
#if defined(MULTIMODULE)
        case MODULE_TYPE_MULTIMODULE:
          required_protocol = PROTOCOL_CHANNELS_MULTIMODULE;
          break;
#endif
#if defined(DSM2)
        case MODULE_TYPE_DSM2:
          required_protocol = limit<uint8_t>(PROTO_DSM2_LP45, PROTO_DSM2_LP45 + g_model.moduleData[EXTERNAL_MODULE].rfProtocol, PROTO_DSM2_DSMX);
          // The module is set to OFF during one second before BIND start
          {
            static tmr10ms_t bindStartTime = 0;
            if (moduleState[EXTERNAL_MODULE].mode == MODULE_MODE_BIND) {
              if (bindStartTime == 0) bindStartTime = get_tmr10ms();
              if ((tmr10ms_t)(get_tmr10ms() - bindStartTime) < 100) {
                required_protocol = PROTOCOL_CHANNELS_NONE;
                break;
              }
            } else {
              bindStartTime = 0;
            }
          }
          break;
#endif
#if defined(CROSSFIRE)
        case MODULE_TYPE_CROSSFIRE:
          required_protocol = PROTOCOL_CHANNELS_CROSSFIRE;
          break;
#endif
        default:
          required_protocol = PROTOCOL_CHANNELS_NONE;
          break;
      }
      break;
  }

  if (s_pulses_paused) {
    required_protocol = PROTOCOL_CHANNELS_NONE;
  }

#if 0
  // will need an EEPROM conversion
  if (moduleState[port].mode == MODULE_OFF) {
    required_protocol = PROTOCOL_CHANNELS_NONE;
  }
#endif

  return required_protocol;
}

void setupPulsesPXX(uint8_t port) {
#if defined(PXX)
#if defined(INTMODULE_USART) && defined(EXTMODULE_USART)
  modulePulsesData[port].pxx_uart.setupFrame(port);
#elif !defined(INTMODULE_USART) && !defined(EXTMODULE_USART)
  modulePulsesData[port].pxx.setupFrame(port);
#else
  if (IS_UART_MODULE(port))
    modulePulsesData[port].pxx_uart.setupFrame(port);
  else
    modulePulsesData[port].pxx.setupFrame(port);
#endif
#endif
}

bool setupPulses(uint8_t port) {
//  TRACE_NOCRLF("sp%d", port);
  bool init_needed = false;
  bool send = false;

  uint8_t required_protocol = getRequiredProtocol(port);

  heartbeat |= (HEART_TIMER_PULSES << port);

  if (moduleState[port].protocol != required_protocol) {
    TRACE("setupPulses %d: %d->%d", port, moduleState[port].protocol, required_protocol);
    init_needed = true;
    switch (moduleState[port].protocol) {  // stop existing protocol hardware

#if defined(CROSSFIRE)
      case PROTOCOL_CHANNELS_CROSSFIRE:
        disable_module_timer(EXTERNAL_MODULE);
        break;
#endif

#if defined(MULTIMODULE)
      case PROTOCOL_CHANNELS_MULTIMODULE:
#endif
      case PROTOCOL_CHANNELS_AFHDS2A_SPI:
        disable_afhds2a(INTERNAL_MODULE);
        break;
#if !defined(PCBI6X)
      case PROTOCOL_CHANNELS_SBUS:
        disable_serial(EXTERNAL_MODULE);
        break;
#endif
      case PROTOCOL_CHANNELS_PPM:
        disable_ppm(EXTERNAL_MODULE);
        break;

      default:
        disable_no_pulses(port);
        break;
    }
    moduleState[port].protocol = required_protocol;
  }

  // Set up output data here
  switch (required_protocol) {

#if !defined(PCBI6X)
    case PROTOCOL_CHANNELS_SBUS:
      setupPulsesSbus();
      scheduleNextMixerCalculation(EXTERNAL_MODULE, SBUS_PERIOD);
      break;
#endif

#if defined(CROSSFIRE)
    case PROTOCOL_CHANNELS_CROSSFIRE:
      if (telemetryProtocol == PROTOCOL_TELEMETRY_CROSSFIRE && !init_needed) {
        ModuleSyncStatus& status = getModuleSyncStatus(EXTERNAL_MODULE);
        if (status.isValid())
          mixerSchedulerSetPeriod(EXTERNAL_MODULE, status.getAdjustedRefreshRate());
        else
          mixerSchedulerSetPeriod(EXTERNAL_MODULE, CROSSFIRE_PERIOD);
        setupPulsesCrossfire();
        send = true;
      }

      break;
#endif

#if defined(MULTIMODULE)
    case PROTOCOL_CHANNELS_MULTIMODULE:
      setupPulsesMultimodule(EXTERNAL_MODULE);
      scheduleNextMixerCalculation(EXTERNAL_MODULE, MULTIMODULE_PERIOD);
      break;
#endif

    case PROTOCOL_CHANNELS_PPM:
      setupPulsesPPMExternalModule();
      mixerSchedulerSetPeriod(EXTERNAL_MODULE, PPM_PERIOD(EXTERNAL_MODULE));
      break;

    case PROTOCOL_CHANNELS_AFHDS2A_SPI:
      // this is kept inside targets/flysky
      // setupPulsesAfhds2aSpi(port);
      break;

    default:
      break;
  }

  if (init_needed) {
    switch (required_protocol) {  // Start new protocol hardware here

#if defined(CROSSFIRE)
      case PROTOCOL_CHANNELS_CROSSFIRE:
        EXTERNAL_MODULE_ON();
        mixerSchedulerSetPeriod(EXTERNAL_MODULE, CROSSFIRE_PERIOD);
        send = true;
        break;
#endif

#if defined(MULTIMODULE)
      case PROTOCOL_CHANNELS_MULTIMODULE:
        init_serial(EXTERNAL_MODULE, MULTIMODULE_BAUDRATE, MULTIMODULE_PERIOD * 2000);
        break;
#endif
#if !defined(PCBI6X)
      case PROTOCOL_CHANNELS_SBUS:
        init_serial(EXTERNAL_MODULE, SBUS_BAUDRATE, SBUS_PERIOD_HALF_US);
        break;
#endif
      case PROTOCOL_CHANNELS_PPM:
        init_ppm(EXTERNAL_MODULE);
        break;
      case PROTOCOL_CHANNELS_AFHDS2A_SPI:
        init_afhds2a(INTERNAL_MODULE);
        mixerSchedulerSetPeriod(INTERNAL_MODULE, AFHDS2A_PERIOD);
        break;
      default:
        init_no_pulses(port);
        mixerSchedulerSetPeriod(port, 0);
        break;
    }
  }
  return send;
}

void setCustomFailsafe(uint8_t moduleIndex) {
  if (moduleIndex < NUM_MODULES) {
    for (int ch = 0; ch < MAX_OUTPUT_CHANNELS; ch++) {
      if (ch < g_model.moduleData[moduleIndex].channelsStart || ch >= sentModuleChannels(moduleIndex) + g_model.moduleData[moduleIndex].channelsStart) {
        g_model.moduleData[moduleIndex].failsafeChannels[ch] = 0;
      } else if (g_model.moduleData[moduleIndex].failsafeChannels[ch] < FAILSAFE_CHANNEL_HOLD) {
        g_model.moduleData[moduleIndex].failsafeChannels[ch] = channelOutputs[ch];
      }
    }
  }
}
