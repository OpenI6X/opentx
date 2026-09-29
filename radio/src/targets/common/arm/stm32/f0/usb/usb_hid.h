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

#ifndef OPENTX_STM32F0_USB_HID_H
#define OPENTX_STM32F0_USB_HID_H

#include "usb_device.h"

extern const UsbClass usbHidClass;

// usbJoystickUpdate() is declared by the target board.h inside an extern "C"
// block, so it is deliberately not repeated here.

#endif // OPENTX_STM32F0_USB_HID_H
