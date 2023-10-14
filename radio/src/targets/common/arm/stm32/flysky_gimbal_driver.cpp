/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
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

STRUCT_HALL HallProtocol = { 0 };

uint8_t HallGetByte(uint8_t * byte)
{
  return aux2SerialRxFifo.pop(*byte);
}

static void Parse_Character(STRUCT_HALL *hallBuffer, unsigned char ch)
{
  switch (hallBuffer->status) {
    case GET_START:
      if (FLYSKY_HALL_PROTOLO_HEAD == ch) {
        hallBuffer->head = FLYSKY_HALL_PROTOLO_HEAD;
        hallBuffer->status = GET_ID;
        hallBuffer->msg_OK = 0;
      }
      break;
    
    case GET_ID:
      hallBuffer->hallID.ID = ch;
      hallBuffer->status = GET_LENGTH;
      break;
    
    case GET_LENGTH:
      hallBuffer->length = ch;
      hallBuffer->dataIndex = 0;
      hallBuffer->status = GET_DATA;
      if (0 == hallBuffer->length) {
        hallBuffer->status = GET_CHECKSUM;
        hallBuffer->checkSum = 0;
      }
      break;
    
    case GET_DATA:
      hallBuffer->data[hallBuffer->dataIndex++] = ch;
      if (hallBuffer->dataIndex >= hallBuffer->length) {
        hallBuffer->checkSum = 0;
        hallBuffer->dataIndex = 0;
        hallBuffer->status = GET_STATE;
      }
      break;
    
    case GET_STATE:
      hallBuffer->checkSum = 0;
      hallBuffer->dataIndex = 0;
      hallBuffer->status = GET_CHECKSUM;
    
    case GET_CHECKSUM:
      hallBuffer->checkSum |= ch << ((hallBuffer->dataIndex++) * 8);
      if (hallBuffer->dataIndex >= 2) {
        hallBuffer->dataIndex = 0;
        hallBuffer->status = CHECKSUM;
      } else {
        break;
      }
    
    case CHECKSUM:
      if (hallBuffer->checkSum ==
          crc16_hw(&hallBuffer->head, hallBuffer->length + 3)) {
        hallBuffer->msg_OK = 1;
      }
      hallBuffer->status = GET_START;
      break; 
  }
}

void flysky_gimbal_loop(void)
{
  uint8_t byte;

  while (HallGetByte(&byte))
  {
    HallProtocol.index++;

    Parse_Character(&HallProtocol, byte);
    if (HallProtocol.msg_OK)
    {
      HallProtocol.msg_OK = 0;
      HallProtocol.stickState = HallProtocol.data[HallProtocol.length - 1];

      switch (HallProtocol.hallID.hall_Id.receiverID)
      {
        case TRANSFER_DIR_TXMCU:
          if (HallProtocol.hallID.hall_Id.packetID == FLYSKY_HALL_RESP_TYPE_VALUES) {
            int16_t* p_values = (int16_t*)HallProtocol.data;
            uint16_t* adcValues = getAnalogValues();
            for (uint8_t i = 0; i < 4; i++) {
              adcValues[i] = FLYSKY_OFFSET_VALUE - (p_values[i] >> 1);
            }
          }
          break;
      }
      //globalData.flyskygimbals = 1;
    }
  }
}

// EdgeTX implements this into a bool function but the idle-callback in this code does not work with it
// A simple void function did the trick
void flysky_gimbal_init()
{
  aux2SerialSetIdleCb(flysky_gimbal_loop);
}