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

#define CROSSFIRE_CH_BITS 11
#define CROSSFIRE_CENTER            0x3E0
#if defined(PPM_CENTER_ADJUSTABLE)
  #define CROSSFIRE_CENTER_CH_OFFSET(ch)            ((2 * limitAddress(ch)->ppmCenter) + 1)  // + 1 is for rouding
#else
  #define CROSSFIRE_CENTER_CH_OFFSET(ch)            (0)
#endif

#if defined(PPM_CENTER_ADJUSTABLE)
#define CROSSFIRE_CENTER_CH_OFFSET(ch) ((2 * limitAddress(ch)->ppmCenter) + 1)  // + 1 is for rouding
#else
#define CROSSFIRE_CENTER_CH_OFFSET(ch) (0)
#endif

uint8_t createCrossfireModelIDFrame(uint8_t* frame) {
  uint8_t* buf = frame;
  *buf++ = UART_SYNC;                               /* device address */
  *buf++ = 8;                                       /* frame length */
  *buf++ = COMMAND_ID;                              /* cmd type */
  *buf++ = MODULE_ADDRESS;                          /* Destination Address */
  *buf++ = RADIO_ADDRESS;                           /* Origin Address */
  *buf++ = SUBCOMMAND_CRSF;                         /* sub command */
  *buf++ = COMMAND_MODEL_SELECT_ID;                 /* command of set model/receiver id */
  *buf++ = g_model.header.modelId[EXTERNAL_MODULE]; /* model ID */
#if defined(PCBI6X)
  *buf++ = crc8_BA_hw(frame + 2, 6);
  *buf++ = crc8_hw(frame + 2, 7);
#else
  *buf++ = crc8_BA(frame + 2, 6);
  *buf++ = crc8(frame + 2, 7);
#endif
  return buf - frame;
}

// Range for pulses (channels output) is [-1024:+1024]
uint8_t createCrossfireChannelsFrame(uint8_t* frame, int16_t* pulses) {
  uint8_t* buf = frame;
  *buf++ = MODULE_ADDRESS;
  *buf++ = 24;  // 1(ID) + 22 + 1(CRC)
  uint8_t* crc_start = buf;
  *buf++ = CHANNELS_ID;
  uint32_t bits = 0;
  uint8_t bitsavailable = 0;
  for (int i = 0; i < CROSSFIRE_CHANNELS_COUNT; i++) {
    uint32_t val = limit(0, CROSSFIRE_CENTER + (CROSSFIRE_CENTER_CH_OFFSET(i) * 4) / 5 + (pulses[i] * 4) / 5, 2 * CROSSFIRE_CENTER);
    bits |= val << bitsavailable;
    bitsavailable += CROSSFIRE_CH_BITS;
    while (bitsavailable >= 8) {
      *buf++ = bits;
      bits >>= 8;
      bitsavailable -= 8;
    }
  }
#if defined(PCBI6X)
  *buf++ = crc8_hw(crc_start, 23);
#else
  *buf++ = crc8(crc_start, 23);
#endif
  return buf - frame;
}

void setupPulsesCrossfire() {
  uint8_t* pulses = modulePulsesData[EXTERNAL_MODULE].crossfire.pulses;

    if (outputTelemetryBufferSize > 0) {
      memcpy(pulses, outputTelemetryBuffer, outputTelemetryBufferSize);
      modulePulsesData[EXTERNAL_MODULE].crossfire.length = outputTelemetryBufferSize;
      outputTelemetryBufferSize = 0;
      outputTelemetryBufferTrigger = 0;
    } else {
    // if (moduleState[EXTERNAL_MODULE].counter == CRSF_FRAME_MODELID) {
    //   modulePulsesData[EXTERNAL_MODULE].crossfire.length = createCrossfireModelIDFrame(pulses);
    //   moduleState[EXTERNAL_MODULE].counter = CRSF_FRAME_MODELID_SENT;
    // } else {
    modulePulsesData[EXTERNAL_MODULE].crossfire.length = createCrossfireChannelsFrame(
        pulses,
        &channelOutputs[g_model.moduleData[EXTERNAL_MODULE].channelsStart]);
    // }
  }
}
