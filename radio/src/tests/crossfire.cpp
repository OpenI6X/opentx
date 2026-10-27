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

#include "gtests.h"
#include "telemetry/telemetry.h"
#include "telemetry/crossfire.h"
#include "crc.h"

#if defined(CROSSFIRE)
uint8_t createCrossfireChannelsFrame(uint8_t * frame, int16_t * pulses);

// Spec-defined part of the frame (0x16 RC Channels Packed): sync byte, type,
// 16 x 11-bit channel packing. Expected bytes computed independently, not
// derived from createCrossfireChannelsFrame() itself.
TEST(Crossfire, createCrossfireChannelsFrame)
{
  MODEL_RESET();

  int16_t pulsesStart[MAX_TRAINER_CHANNELS];
  uint8_t crossfire[CROSSFIRE_FRAME_MAXLEN];

  memset(crossfire, 0, sizeof(crossfire));
  for (int i=0; i<MAX_TRAINER_CHANNELS; i++) {
    pulsesStart[i] = -1024 + (2048 / MAX_TRAINER_CHANNELS) * i;
  }

  createCrossfireChannelsFrame(crossfire, pulsesStart);

  ASSERT_EQ(crossfire[0], CSRF_ADDRESS_TX);
  ASSERT_EQ(crossfire[2], CHANNELS_ID);

  const uint8_t expectedChannelData[22] = {
    0xAD, 0xA0, 0x88, 0x5E, 0xC0, 0x73, 0xA4, 0x56, 0x51, 0x4C, 0x6F,
    0xE0, 0x33, 0x22, 0x2B, 0x27, 0x9A, 0x57, 0xF0, 0x1A, 0x99, 0xD5
  };
  ASSERT_EQ(memcmp(&crossfire[3], expectedChannelData, sizeof(expectedChannelData)), 0);
}

// Status byte after the 0x16 payload is an ExpressLRS extension, not TBS CRSF
// spec (semantics per ExpressLRS's TXModuleEndpoint.cpp / crsf_protocol.h).
// Frame is always 25 bytes (1 ID + 22 channel data + 1 status + 1 CRC);
// bit 0 = commanded armed status (Switch mode only), bit 1 = arming mode is CH5.
TEST(Crossfire, ExpressLRSArmingExtension_CH5Mode)
{
  MODEL_RESET();

  int16_t pulsesStart[MAX_TRAINER_CHANNELS];
  uint8_t crossfire[CROSSFIRE_FRAME_MAXLEN];

  memset(crossfire, 0, sizeof(crossfire));
  for (int i=0; i<MAX_TRAINER_CHANNELS; i++) {
    pulsesStart[i] = -1024 + (2048 / MAX_TRAINER_CHANNELS) * i;
  }

  g_model.moduleData[EXTERNAL_MODULE].crsf.crsfArmingMode = ARMING_MODE_CH5;

  uint8_t len = createCrossfireChannelsFrame(crossfire, pulsesStart);

  ASSERT_EQ(len, 27);
  ASSERT_EQ(crossfire[0], CSRF_ADDRESS_TX);
  ASSERT_EQ(crossfire[1], 25);
  ASSERT_EQ(crossfire[2], CHANNELS_ID);
  ASSERT_EQ(crossfire[25], 0x02); // bit 1: arming mode CH5

  uint8_t crc = crc8(&crossfire[2], 24);
  ASSERT_EQ(crossfire[26], crc);
}

TEST(Crossfire, ExpressLRSArmingExtension_SwitchMode)
{
  MODEL_RESET();

  int16_t pulsesStart[MAX_TRAINER_CHANNELS];
  uint8_t crossfire[CROSSFIRE_FRAME_MAXLEN];

  memset(crossfire, 0, sizeof(crossfire));
  for (int i=0; i<MAX_TRAINER_CHANNELS; i++) {
    pulsesStart[i] = -1024 + (2048 / MAX_TRAINER_CHANNELS) * i;
  }

  g_model.moduleData[EXTERNAL_MODULE].crsf.crsfArmingMode = ARMING_MODE_SWITCH;
  g_model.moduleData[EXTERNAL_MODULE].crsf.crsfArmingTrigger = SWSRC_NONE;

  uint8_t len = createCrossfireChannelsFrame(crossfire, pulsesStart);

  ASSERT_EQ(len, 27);
  ASSERT_EQ(crossfire[0], CSRF_ADDRESS_TX);
  ASSERT_EQ(crossfire[1], 25);
  ASSERT_EQ(crossfire[2], CHANNELS_ID);
  ASSERT_EQ(crossfire[25], 0); // SWSRC_NONE -> not armed, bit 1 clear (Switch mode)

  uint8_t crc = crc8(&crossfire[2], 24);
  ASSERT_EQ(crossfire[26], crc);
}

TEST(Crossfire, crc8)
{
  uint8_t frame[] = { 0x00, 0x0C, 0x14, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x03, 0x00, 0x00, 0x00, 0xF4 };
  uint8_t crc = crc8(&frame[2], frame[1]-1);
  ASSERT_EQ(frame[frame[1]+1], crc);
}
#endif

