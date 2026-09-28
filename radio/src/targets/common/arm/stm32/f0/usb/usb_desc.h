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

#ifndef OPENTX_STM32F0_USB_DESC_H
#define OPENTX_STM32F0_USB_DESC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// String descriptor indexes
enum {
  USB_STR_LANGID = 0,
  USB_STR_MANUFACTURER,
  USB_STR_PRODUCT,
  USB_STR_SERIAL,
  USB_STR_CONFIG,
  USB_STR_INTERFACE
};

const uint8_t * usbGetDeviceDesc(uint16_t * len);
const uint8_t * usbGetStringDesc(uint8_t index, uint16_t * len);

#ifdef __cplusplus
}
#endif

#endif // OPENTX_STM32F0_USB_DESC_H
