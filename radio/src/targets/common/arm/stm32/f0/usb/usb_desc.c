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
 * Device descriptor and the per mode string descriptors.
 *
 * The configuration descriptor is mode specific and lives with each class
 * module; the USB identity (VID/PID and product name) is chosen here.
 */

#include "usb_desc.h"
#include "usb_driver.h"
#include "board.h"
#include <string.h>

#include <string.h>

#define USB_VID_STM       0x0483    // STM vendor id
#define USB_VID_PID_CODES 0x1209    // https://pid.codes
#define USB_LANGID        0x0409

#if defined(BOOT)
  #define USB_PRODUCT_MSC  USB_NAME " Bootloader"
#else
  #define USB_PRODUCT_MSC  USB_NAME " Storage"
#endif
#define USB_PRODUCT_HID   USB_NAME " Joystick"
#define USB_PRODUCT_CDC   USB_NAME " Serial"

#define USB_CONFIG_MSC    "MSC Config"
#define USB_CONFIG_HID    "HID Config"
#define USB_CONFIG_CDC    "VSP Config"

#define USB_IFACE_MSC     "MSC Interface"
#define USB_IFACE_HID     "HID Interface"
#define USB_IFACE_CDC     "VSP Interface"

#define USB_MFR_STRING     "OpenTX"
#define USB_SERIALNUMBER  "01"

// The longest string is "FS-i6X Bootloader" (17 chars) -> 17 * 2 + 2 = 36.
// The same buffer also carries the 18 byte device descriptor: only one
// descriptor is ever on the wire, so they can share (as the previous
// implementation did with USBD_StrDesc).
#define USB_STR_DESC_SIZ  40

static uint8_t descBuf[USB_STR_DESC_SIZ];

const uint8_t * usbGetDeviceDesc(uint16_t * len)
{
  uint8_t * devDesc = descBuf;
  uint16_t vid, pid;

  switch (getSelectedUsbMode()) {
    case USB_JOYSTICK_MODE:
      vid = USB_VID_PID_CODES;
      pid = 0x4F54;              // OpenTX assigned PID
      break;
#if defined(USB_SERIAL)
    case USB_SERIAL_MODE:
      vid = USB_VID_STM;        // required by the ST Windows CDC driver
      pid = 0x5740;
      break;
#endif
    default:
      vid = USB_VID_PID_CODES;
      pid = 0x5720;
      break;
  }

  devDesc[0] = 18;
  devDesc[1] = 0x01;             // DEVICE
  devDesc[2] = 0x00;             // bcdUSB 2.00
  devDesc[3] = 0x02;
  devDesc[4] = 0x00;             // class / subclass / protocol: defined by the interface
  devDesc[5] = 0x00;
  devDesc[6] = 0x00;
  devDesc[7] = 64;               // bMaxPacketSize0
  devDesc[8] = (uint8_t)(vid & 0xFF);
  devDesc[9] = (uint8_t)(vid >> 8);
  devDesc[10] = (uint8_t)(pid & 0xFF);
  devDesc[11] = (uint8_t)(pid >> 8);
  devDesc[12] = 0x00;            // bcdDevice 2.00
  devDesc[13] = 0x02;
  devDesc[14] = USB_STR_MANUFACTURER;
  devDesc[15] = USB_STR_PRODUCT;
  devDesc[16] = USB_STR_SERIAL;
  devDesc[17] = 1;               // bNumConfigurations

  *len = 18;
  return descBuf;
}

// Convert an ASCII string into a USB string descriptor in place.  The buffer is
// shared, exactly as with the previous implementation, because only one string
// is ever on the wire at a time.
static const uint8_t * usbMakeString(const char * s, uint16_t * len)
{
  uint8_t n = 0;
  descBuf[0] = 0;
  descBuf[1] = 0x03;             // STRING
  while (s[n] && (2 + 2 * n + 2) <= USB_STR_DESC_SIZ) {
    descBuf[2 + 2 * n] = (uint8_t)s[n];
    descBuf[3 + 2 * n] = 0;
    n++;
  }
  descBuf[0] = (uint8_t)(2 + 2 * n);
  *len = descBuf[0];
  return descBuf;
}

const uint8_t * usbGetStringDesc(uint8_t index, uint16_t * len)
{
  switch (index) {
    case USB_STR_LANGID:
      descBuf[0] = 4;
      descBuf[1] = 0x03;
      descBuf[2] = (uint8_t)(USB_LANGID & 0xFF);
      descBuf[3] = (uint8_t)(USB_LANGID >> 8);
      *len = 4;
      return descBuf;

    case USB_STR_MANUFACTURER:
      return usbMakeString(USB_MFR_STRING, len);

    case USB_STR_SERIAL:
      return usbMakeString(USB_SERIALNUMBER, len);

    case USB_STR_PRODUCT:
    case USB_STR_CONFIG:
    case USB_STR_INTERFACE:
      switch (getSelectedUsbMode()) {
        case USB_JOYSTICK_MODE:
          if (index == USB_STR_PRODUCT) return usbMakeString(USB_PRODUCT_HID, len);
          if (index == USB_STR_CONFIG)  return usbMakeString(USB_CONFIG_HID, len);
          return usbMakeString(USB_IFACE_HID, len);
#if defined(USB_SERIAL)
        case USB_SERIAL_MODE:
          if (index == USB_STR_PRODUCT) return usbMakeString(USB_PRODUCT_CDC, len);
          if (index == USB_STR_CONFIG)  return usbMakeString(USB_CONFIG_CDC, len);
          return usbMakeString(USB_IFACE_CDC, len);
#endif
        default:
          if (index == USB_STR_PRODUCT) return usbMakeString(USB_PRODUCT_MSC, len);
          if (index == USB_STR_CONFIG)  return usbMakeString(USB_CONFIG_MSC, len);
          return usbMakeString(USB_IFACE_MSC, len);
      }

    default:
      *len = 0;
      return descBuf;
  }
}
