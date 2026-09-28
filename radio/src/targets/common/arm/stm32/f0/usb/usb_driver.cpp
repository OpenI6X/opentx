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

/*
 * Top level USB API for the STM32F0 targets, matching the interface the rest
 * of the firmware expects (see targets/common/arm/stm32/usb_driver.h).
 *
 * The F2/F4 targets keep using usb_driver.cpp together with ST's OTG library;
 * this file replaces it for STM32F0 only.
 */

// opentx.h first: it pulls in board.h, which declares the usb_driver.h API
// inside an extern "C" block, so the definitions below pick up C linkage.
#include "opentx.h"
#include "debug.h"

#include "usb_device.h"
#include "usb_hid.h"
#include "usb_msd.h"
#include "usb_cdc.h"

static bool usbDriverStarted = false;

#if defined(BOOT)
static usbMode selectedUsbMode = USB_MASS_STORAGE_MODE;
#else
static usbMode selectedUsbMode = USB_UNSELECTED_MODE;
#endif

int getSelectedUsbMode()
{
  return selectedUsbMode;
}

void setSelectedUsbMode(int mode)
{
  selectedUsbMode = usbMode (mode);
}

int usbPlugged()
{
  // if connected prevent false reads on USB DM pin
  if (usbStarted()) {
    return 1;
  }

  static uint8_t debouncedState = 0;
  static uint8_t lastState = 0;

  uint8_t state = LL_GPIO_IsInputPinSet(USB_GPIO, USB_GPIO_PIN_DM) ? 0 : 1;

  if (state == lastState)
    debouncedState = state;
  else
    lastState = state;

  return debouncedState;
}

extern "C" void USB_IRQHandler()
{
  usbDeviceIsr();
}

void usbInit()
{
  usbDeviceInit();
  usbDriverStarted = false;
}

void usbStart()
{
  switch (getSelectedUsbMode()) {
#if !defined(BOOT)
    case USB_JOYSTICK_MODE:
      usbDeviceStart(&usbHidClass);
      break;
#endif
#if defined(USB_SERIAL)
    case USB_SERIAL_MODE:
      usbDeviceStart(&usbCdcClass);
      break;
#endif
#if defined(USB_MSD)
    default:
    case USB_MASS_STORAGE_MODE:
      usbDeviceStart(&usbMscClass);
      break;
#endif
  }
  usbDriverStarted = true;
}

void usbStop()
{
  usbDriverStarted = false;
  usbDeviceStop();
}

bool usbStarted()
{
  return usbDriverStarted;
}
