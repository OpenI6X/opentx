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
 * USB HID joystick: exposes the 8 main channels as 11 bit analog axes plus
 * digital buttons derived from the remaining channel outputs.
 */

#include "usb_device.h"
#include "usb_hid.h"
#include "board.h"
#include "opentx.h"

#define HID_EP_IN        1
#define HID_IN_PACKET    18      // 2 button bytes + 8 axes of 2 bytes

// Report descriptor: 16 digital buttons and 8 11 bit axes.
static const uint8_t hidReportDesc[] = {
  0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
  0x09, 0x05,                    // USAGE (Game Pad)
  0xa1, 0x01,                    // COLLECTION (Application)
  0xa1, 0x00,                    //   COLLECTION (Physical)
  0x05, 0x09,                    //     USAGE_PAGE (Button)
  0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
  0x29, 0x10,                    //     USAGE_MAXIMUM (Button 16)
  0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
  0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
  0x95, 0x10,                    //     REPORT_COUNT (16)
  0x75, 0x01,                    //     REPORT_SIZE (1)
  0x81, 0x02,                    //     INPUT (Data,Var,Abs)
  0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
  0x09, 0x30,                    //     USAGE (X)
  0x09, 0x31,                    //     USAGE (Y)
  0x09, 0x32,                    //     USAGE (Z)
  0x09, 0x33,                    //     USAGE (Rx)
  0x09, 0x34,                    //     USAGE (Ry)
  0x09, 0x35,                    //     USAGE (Rz)
  0x09, 0x36,                    //     USAGE (Slider)
  0x09, 0x37,                    //     USAGE (Slider)
  0x16, 0x00, 0x00,              //     LOGICAL_MINIMUM (0)
  0x26, 0xff, 0x07,              //     LOGICAL_MAXIMUM (2047)
  0x75, 0x10,                    //     REPORT_SIZE (16)
  0x95, 0x08,                    //     REPORT_COUNT (8)
  0x81, 0x02,                    //     INPUT (Data,Var,Abs)
  0xc0,                          //   END_COLLECTION
  0xc0                           // END_COLLECTION
};

#define HID_DESC_SIZ     34

static const uint8_t hidCfgDesc[HID_DESC_SIZ] = {
  0x09, 0x02,                    // Configuration, total length below
  (HID_DESC_SIZ) & 0xFF,
  (HID_DESC_SIZ) >> 8,
  0x01,                          // bNumInterfaces
  0x01,                          // bConfigurationValue
  0x00,                          // iConfiguration
  0xC0,                          // bmAttributes: self powered
  0x32,                          // bMaxPower 100 mA

  0x09, 0x04,                    // Interface
  0x00,                          // bInterfaceNumber
  0x00,                          // bAlternateSetting
  0x01,                          // bNumEndpoints
  0x03,                          // bInterfaceClass: HID
  0x00,                          // bInterfaceSubClass
  0x00,                          // bInterfaceProtocol
  0x00,                          // iInterface

  0x09, 0x21,                    // HID descriptor
  0x11, 0x01,                    // bcdHID 1.11
  0x00,                          // bCountryCode
  0x01,                          // bNumDescriptors
  0x22,                          // bDescriptorType: report
  (sizeof(hidReportDesc)) & 0xFF,
  (sizeof(hidReportDesc)) >> 8,

  0x07, 0x05,                    // Endpoint
  0x81,                          // bEndpointAddress: IN
  0x03,                          // bmAttributes: interrupt
  HID_IN_PACKET, 0x00,           // wMaxPacketSize
  0x04                           // bInterval 4 ms
};

// 0x21 = HID descriptor, 0x22 = report descriptor
#define HID_DESC_TYPE    0x21
#define HID_REPORT_DESC  0x22

static const uint8_t * hidGetConfigDesc(uint16_t * len)
{
  *len = HID_DESC_SIZ;
  return hidCfgDesc;
}

static void hidInit(void)
{
  usbEpOpen(HID_EP_IN, true, 3 /* interrupt */, HID_IN_PACKET);
}

static void hidDeinit(void)
{
  usbEpClose(HID_EP_IN, true);
}

static bool hidSetup(const UsbSetupReq * req)
{
  uint16_t len;
  const uint8_t * buf;

  // GET_DESCRIPTOR for the report and HID descriptors is forwarded here by the
  // core, which only knows the device, configuration and string types.
  if (req->bRequest != 0x06) {
    return false;                       // no other HID class request is used
  }

  if ((req->wValue >> 8) == HID_REPORT_DESC) {
    buf = hidReportDesc;
    len = (uint16_t)sizeof(hidReportDesc);
  }
  else if ((req->wValue >> 8) == HID_DESC_TYPE) {
    buf = hidCfgDesc + 0x12;            // the HID descriptor inside the config
    len = 9;
  }
  else {
    return false;
  }

  usbCtlSendData(buf, (uint16_t)((len < req->wLength) ? len : req->wLength));
  return true;
}

static void hidDataIn(uint8_t ep)
{
}

static void hidDataOut(uint8_t ep, uint16_t len)
{
}

static void hidSof(void)
{
}

const UsbClass usbHidClass = {
  hidGetConfigDesc,
  hidInit,
  hidDeinit,
  hidSetup,
  hidDataIn,
  hidDataOut,
  hidSof
};

// ---------------------------------------------------------------------------
// Report generation, called from the mixer task
// ---------------------------------------------------------------------------

void usbJoystickUpdate(void)
{
  static uint8_t report[HID_IN_PACKET];

  if (!usbIsConfigured()) {
    return;
  }

  UsbEp * ep = usbEpGet(HID_EP_IN, true);
  if (!ep || !ep->open) {
    return;
  }

  // Skip if the previous report has not been picked up by the host yet
  if (ep->len) {
    return;
  }

  report[0] = 0;
  report[1] = 0;
  for (int i = 0; i < 8; ++i) {
    if (channelOutputs[i + 8] > 0) {
      report[0] |= (1 << i);
    }
#if MAX_OUTPUT_CHANNELS >= 24
    if (channelOutputs[i + 16] > 0) {
      report[1] |= (1 << i);
    }
#endif
  }

  for (int i = 0; i < 8; ++i) {
    int value = channelOutputs[i] + 1024;
    if (value > 2047) {
      value = 2047;
    }
    else if (value < 0) {
      value = 0;
    }
    report[2 + i * 2] = (uint8_t)(value & 0xFF);
    report[3 + i * 2] = (uint8_t)((value >> 8) & 0x07);
  }

  usbEpStartTx(HID_EP_IN, report, HID_IN_PACKET);
}
