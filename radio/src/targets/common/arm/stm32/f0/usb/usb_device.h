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
 * Compact full speed USB device stack for the STM32F0x2.
 *
 * Replaces ST's usb_core / usb_dcd / usb_dcd_int / usbd_core / usbd_req /
 * usbd_ioreq with a single device core plus one module per class.
 */

#ifndef OPENTX_STM32F0_USB_DEVICE_H
#define OPENTX_STM32F0_USB_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USB_EP_COUNT     3     // EP0 + EP1 + EP2, which is all any mode uses
#define USB_EP0_MPS      64

// Device state
enum {
  USB_STATE_DEFAULT = 0,
  USB_STATE_ADDRESSED,
  USB_STATE_CONFIGURED
};

// Control endpoint transfer stage
enum {
  USB_EP0_IDLE = 0,
  USB_EP0_DATA_IN,
  USB_EP0_DATA_OUT,
  USB_EP0_STATUS_IN,
  USB_EP0_STATUS_OUT
};

typedef struct
{
  uint8_t  bmRequestType;
  uint8_t  bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} UsbSetupReq;

typedef struct
{
  uint8_t * buf;      // walking pointer into the transfer buffer
  uint16_t len;       // bytes still expected/sent for the whole transfer
  uint16_t total;     // on receive: bytes actually received so far
  uint16_t maxpkt;
  uint16_t pma;       // packet memory offset of this direction's buffer
  uint8_t  isStall;
  uint8_t  open;
} UsbEp;

// Implemented by usb_hid.c / usb_msd.c / usb_cdc.c.  Only one class is ever
// active (the mode is chosen when USB starts), so a single vtable suffices.
typedef struct
{
  const uint8_t * (*getConfigDesc)(uint16_t * len);
  void (*init)(void);
  void (*deinit)(void);
  bool (*setup)(const UsbSetupReq * req);   // true if the request was handled
  void (*dataIn)(uint8_t ep);
  void (*dataOut)(uint8_t ep, uint16_t len);
  void (*sof)(void);
} UsbClass;

// --- core (usb_device.c) ---------------------------------------------------

void usbDeviceInit(void);
void usbDeviceStart(const UsbClass * cls);
void usbDeviceStop(void);
void usbDeviceIsr(void);

uint8_t usbEpOpen(uint8_t epNum, bool isIn, uint8_t type, uint8_t maxpkt);
void usbEpClose(uint8_t epNum, bool isIn);
void usbEpStartTx(uint8_t epNum, const uint8_t * buf, uint16_t len);
void usbEpStartRx(uint8_t epNum, uint8_t * buf, uint16_t len);
void usbEpStall(uint8_t epNum, bool isIn);
void usbEpClearStall(uint8_t epNum, bool isIn);
UsbEp * usbEpGet(uint8_t epNum, bool isIn);
void ep0OrEpArmRx(uint8_t epNum, uint16_t count);

bool usbIsConfigured(void);

// Control pipe helpers, used by the class modules from their setup handlers
void usbCtlSendData(const uint8_t * buf, uint16_t len);
void usbCtlSendStatus(void);
void usbCtlPrepareRx(uint8_t * buf, uint16_t len);
void usbCtlError(void);          // stall the control IN endpoint

#ifdef __cplusplus
}
#endif

#endif // OPENTX_STM32F0_USB_DEVICE_H
