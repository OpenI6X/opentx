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
ModulePulsesData modulePulsesData[NUM_MODULES] __DMA;
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
    TRACE("setupPulses %d change %d->%d", port, moduleState[port].protocol, required_protocol);
    init_needed = true;
    switch (moduleState[port].protocol) {  // stop existing protocol hardware
#if defined(PXX)
      case PROTO_PXX:
        disable_pxx(port);
        break;
#endif
#if defined(DSM2)
      case PROTO_DSM2_LP45:
      case PROTO_DSM2_DSM2:
      case PROTO_DSM2_DSMX:
        disable_serial(port);
        break;
#endif

#if defined(CROSSFIRE)
      case PROTOCOL_CHANNELS_CROSSFIRE:
        disable_module_timer(port);
        break;
#endif

#if defined(PXX2)
      case PROTO_PXX2:
        disable_module_timer(port);
        break;
#endif

#if defined(MULTIMODULE)
      case PROTOCOL_CHANNELS_MULTIMODULE:
#endif
      case PROTOCOL_CHANNELS_AFHDS2A_SPI:
        disable_afhds2a(port);
        break;
#if !defined(PCBI6X)
      case PROTOCOL_CHANNELS_SBUS:
        disable_serial(port);
        break;
#endif
      case PROTOCOL_CHANNELS_PPM:
        disable_ppm(port);
        break;

      default:
        disable_no_pulses(port);
        break;
    }
    moduleState[port].protocol = required_protocol;
  }

  // Set up output data here
  switch (required_protocol) {
#if defined(PXX)
    case PROTO_PXX:
      setupPulsesPXX(port);
      scheduleNextMixerCalculation(port, PXX_PERIOD);
      break;
#endif

#if !defined(PCBI6X)
    case PROTOCOL_CHANNELS_SBUS:
      setupPulsesSbus(port);
      scheduleNextMixerCalculation(port, SBUS_PERIOD);
      break;
#endif

#if defined(DSM2)
    case PROTO_DSM2_LP45:
    case PROTO_DSM2_DSM2:
    case PROTO_DSM2_DSMX:
      setupPulsesDSM2(port);
      scheduleNextMixerCalculation(port, DSM2_PERIOD);
      break;
#endif

#if defined(CROSSFIRE)
    case PROTOCOL_CHANNELS_CROSSFIRE:
      if (telemetryProtocol == PROTOCOL_PULSES_CROSSFIRE && !init_needed) {
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

#if defined(PXX2)
    case PROTO_PXX2:
      if (telemetryProtocol == PROTOCOL_FRSKY_SPORT && !init_needed) {
        modulePulsesData[port].pxx2.setupFrame(port);
        sportSendBuffer(modulePulsesData[port].pxx2.getData(), modulePulsesData[port].pxx2.getSize());
      }
      scheduleNextMixerCalculation(port, PXX2_PERIOD);
      break;
#endif

#if defined(MULTIMODULE)
    case PROTOCOL_CHANNELS_MULTIMODULE:
      setupPulsesMultimodule(port);
      scheduleNextMixerCalculation(port, MULTIMODULE_PERIOD);
      break;
#endif

    case PROTOCOL_CHANNELS_PPM:
#if defined(PCBSKY9X)
    case PROTOCOL_CHANNELS_NONE:
#endif
      setupPulsesPPMModule(port);
      mixerSchedulerSetPeriod(port, PPM_PERIOD(port));
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
#if defined(PXX)
      case PROTO_PXX:
        init_pxx(port);
        break;
#endif
#if defined(DSM2)
      case PROTO_DSM2_LP45:
      case PROTO_DSM2_DSM2:
      case PROTO_DSM2_DSMX:
        init_serial(port, DSM2_BAUDRATE, DSM2_PERIOD * 2000);
        break;
#endif

#if defined(CROSSFIRE)
      case PROTOCOL_CHANNELS_CROSSFIRE:
        EXTERNAL_MODULE_ON();
        mixerSchedulerSetPeriod(EXTERNAL_MODULE, CROSSFIRE_PERIOD);
        send = true;
        break;
#endif

#if defined(PXX2)
      case PROTO_PXX2:
        init_module_timer(port, PXX2_PERIOD, true);
        break;
#endif

#if defined(MULTIMODULE)
      case PROTOCOL_CHANNELS_MULTIMODULE:
        init_serial(port, MULTIMODULE_BAUDRATE, MULTIMODULE_PERIOD * 2000);
        break;
#endif
#if !defined(PCBI6X)
      case PROTOCOL_CHANNELS_SBUS:
        init_serial(port, SBUS_BAUDRATE, SBUS_PERIOD_HALF_US);
        break;
#endif
      case PROTOCOL_CHANNELS_PPM:
        init_ppm(port);
        break;
      case PROTOCOL_CHANNELS_AFHDS2A_SPI:
        init_afhds2a(port);
        mixerSchedulerSetPeriod(INTERNAL_MODULE, 3850);
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
