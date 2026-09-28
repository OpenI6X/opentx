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

#ifndef OPENTX_STM32F0_USB_STORAGE_H
#define OPENTX_STM32F0_USB_STORAGE_H

#include <stdint.h>

/*
 * Block device back end for the USB mass storage class.
 *
 * Implemented by targets/common/arm/stm32/usbd_storage_msd.cpp, which exposes
 * a FAT12 virtual disk over the internal flash (and EEPROM, and the SD card
 * when that option is built in).
 */

#ifdef __cplusplus
extern "C" {
#endif

int8_t  STORAGE_GetCapacity(uint8_t lun, uint32_t * blockNum, uint32_t * blockSize);
int8_t  STORAGE_IsReady(uint8_t lun);
int8_t  STORAGE_IsWriteProtected(uint8_t lun);
int8_t  STORAGE_Read(uint8_t lun, uint8_t * buf, uint32_t blkAddr, uint16_t blkLen);
int8_t  STORAGE_Write(uint8_t lun, uint8_t * buf, uint32_t blkAddr, uint16_t blkLen);
int8_t  STORAGE_GetMaxLun(void);

extern const unsigned char STORAGE_Inquirydata[];

#ifdef __cplusplus
}
#endif

#endif // OPENTX_STM32F0_USB_STORAGE_H
