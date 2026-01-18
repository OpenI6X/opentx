/*
 * Copyright (C) OpenI6X
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

#ifndef _CRC_DRIVER_H_
#define _CRC_DRIVER_H_

#define CRC8_POL_D5   0xD5D5D5D5
#define CRC8_POL_BA   0xBABABABA
#define CRC8_INIT_VAL 0x00

#define CRC16_POL_1021 0x1021
#define CRC16_INIT_VAL 0xFFFF

uint8_t crc8(const uint8_t * ptr, uint32_t len);
uint8_t crc8_BA(const uint8_t * ptr, uint32_t len);
uint16_t crc16(const uint8_t * ptr, uint32_t len);

#endif // _CRC_DRIVER_H_