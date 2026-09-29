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
 * USB CDC virtual serial port.
 *
 * Replaces usbd_cdc_core.c.  The ST version keeps a second bookkeeping
 * structure (APP_Rx_length / USB_Tx_State / last_packet) around the TX ring;
 * here the ring is drained straight from the IN endpoint completion, so those
 * are gone.
 *
 * TX is fed from the SOF hook rather than immediately, matching the original:
 * the host polls once per frame and several 1 ms telemetry protocols (and the
 * SBUS trainer) rely on the pacing.
 */

#include "usb_device.h"
#include "usb_cdc.h"
#include "board.h"
#include "opentx.h"
#include "serial_buffer_union.h"

#include <string.h>

#if defined(CLI)
#include "cli.h"
#endif

#define CDC_EP_IN        0x81
#define CDC_EP_OUT       0x01
#define CDC_EP_CMD_IN    0x82
#define CDC_EP_IN_NUM    1
#define CDC_EP_OUT_NUM   1
#define CDC_EP_CMD_NUM   2
#define CDC_MPS          64
#define CDC_CMD_MPS      8
#define CDC_IN_INTERVAL  4       // frames between IN transfers

// VID 0x0483 / PID 0x5740 is what the ST CDC driver for Windows binds to.
static const uint8_t cdcCfgDesc[67] = {
  0x09, 0x02, 67, 0x00,         // Configuration, total length
  0x02, 0x01, 0x00, 0xC0, 0x32, // 2 interfaces, self powered

  0x09, 0x04,                   // Communications interface
  0x00, 0x00, 0x01,
  0x02, 0x02, 0x01,             // CDC, ACM, AT commands
  0x00,

  0x05, 0x24, 0x00, 0x10, 0x01, // header functional
  0x05, 0x24, 0x01, 0x00, 0x01, // call management, data interface 1
  0x04, 0x24, 0x02, 0x02,       // ACM: line coding + serial state
  0x05, 0x24, 0x06, 0x00, 0x01, // union: master 0, slave 1

  0x07, 0x05, CDC_EP_CMD_IN, 0x03, CDC_CMD_MPS, 0x00, 0xFF,

  0x09, 0x04,                   // Data class interface
  0x01, 0x00, 0x02,
  0x0A, 0x00, 0x00,             // CDC data
  0x00,

  0x07, 0x05, CDC_EP_OUT, 0x02, CDC_MPS, 0x00, 0x00,
  0x07, 0x05, CDC_EP_IN,  0x02, CDC_MPS, 0x00, 0x00
};

#define CDC_DESC_TYPE   0x24    // CS_INTERFACE

// TX ring lives in the shared serialBuffer union (serial_buffer_union.h) to
// save RAM: USB CDC owns it while in serial mode, the AUX serial TX fifo
// otherwise.  Each side resets its indices on init (cdcInit / auxSerialInit).
static uint8_t rxBuf[CDC_MPS];
static uint8_t cmdBuf[CDC_CMD_MPS];

volatile uint32_t APP_Tx_ptr_in = 0;     // written by usbSerialPutc()
volatile uint32_t APP_Rx_ptr_out = 0;    // drained by the IN endpoint

static bool cdcConnected = false;
static uint8_t frameCount = 0;
static uint8_t pendingCmd = 0xFF;
static uint8_t pendingLen = 0;

// Push as much of the ring as fits into one 64 byte packet.
static void cdcTxPump(void)
{
  if (!cdcConnected) {
    return;
  }

  UsbEp * ep = usbEpGet(CDC_EP_IN_NUM, true);
  if (!ep || !ep->open || ep->len) {
    return;                               // a packet is still in flight
  }

  uint32_t avail = (APP_Tx_ptr_in + APP_TX_DATA_SIZE - APP_Rx_ptr_out) % APP_TX_DATA_SIZE;
  if (!avail) {
    return;
  }

  uint16_t n = (uint16_t)((avail > CDC_MPS) ? CDC_MPS : avail);
  // Do not wrap within one packet; usbPmaWrite() takes a contiguous buffer
  if (APP_Rx_ptr_out + n > APP_TX_DATA_SIZE) {
    n = (uint16_t)(APP_TX_DATA_SIZE - APP_Rx_ptr_out);
  }

  usbEpStartTx(CDC_EP_IN_NUM, &serialBuffer.txBuf[APP_Rx_ptr_out], n);
  APP_Rx_ptr_out = (APP_Rx_ptr_out + n) % APP_TX_DATA_SIZE;
}

// ---------------------------------------------------------------------------
// Class callbacks
// ---------------------------------------------------------------------------

static const uint8_t * cdcGetConfigDesc(uint16_t * len)
{
  *len = sizeof(cdcCfgDesc);
  return cdcCfgDesc;
}

static void cdcInit(void)
{
  APP_Tx_ptr_in = 0;
  APP_Rx_ptr_out = 0;
  frameCount = 0;
  pendingCmd = 0xFF;

  usbEpOpen(CDC_EP_IN_NUM, true, 2 /* bulk */, CDC_MPS);
  usbEpOpen(CDC_EP_OUT_NUM, false, 2 /* bulk */, CDC_MPS);
  usbEpOpen(CDC_EP_CMD_NUM, true, 3 /* interrupt */, CDC_CMD_MPS);
  usbEpStartRx(CDC_EP_OUT_NUM, rxBuf, CDC_MPS);

  cdcConnected = true;
}

static void cdcDeinit(void)
{
  usbEpClose(CDC_EP_IN_NUM, true);
  usbEpClose(CDC_EP_OUT_NUM, false);
  usbEpClose(CDC_EP_CMD_NUM, true);
  cdcConnected = false;
}

static bool cdcSetup(const UsbSetupReq * req)
{
  if ((req->bmRequestType & 0x60) == 0x20) {
    // Only SET_LINE_CODING carries data worth acting on; the line coding and
    // control line state are both fixed at 115200 8N1 with DTR/RTS asserted,
    // which is what the radio's protocols expect.
    if (req->bRequest == 0x20 && req->wLength == 7 && !(req->bmRequestType & 0x80)) {
      pendingCmd = req->bRequest;
      pendingLen = (uint8_t)req->wLength;
      usbCtlPrepareRx(cmdBuf, req->wLength);
      return true;
    }
    if (req->wLength == 0) {
      usbCtlSendStatus();
      return true;
    }
    return false;
  }

  if (req->bRequest == 0x06 && (req->wValue >> 8) == CDC_DESC_TYPE) {
    // The functional descriptors start right after the configuration and the
    // first interface descriptor, and the host asks for them by index 0.
    static const uint8_t funcs[] = { 0x05, 0x24, 0x00, 0x10, 0x01 };
    usbCtlSendData(funcs, (req->wLength < sizeof(funcs)) ? (uint16_t)req->wLength : sizeof(funcs));
    return true;
  }
  return false;
}

static void cdcDataIn(uint8_t ep)
{
  if (ep == CDC_EP_CMD_NUM) {
    return;                               // notification endpoint, unused
  }
  cdcTxPump();
}

static void cdcDataOut(uint8_t ep, uint16_t len)
{
  if (ep == 0) {
    // Control OUT data stage finished; the line coding is fixed so there is
    // nothing to store.
    pendingCmd = 0xFF;
    return;
  }

#if defined(CLI)
  for (uint16_t i = 0; i < len; i++) {
    cliRxFifo.push(rxBuf[i]);
  }
#endif

  usbEpStartRx(CDC_EP_OUT_NUM, rxBuf, CDC_MPS);
}

static void cdcSof(void)
{
  if (++frameCount < CDC_IN_INTERVAL) {
    return;
  }
  frameCount = 0;
  cdcTxPump();
}

const UsbClass usbCdcClass = {
  cdcGetConfigDesc,
  cdcInit,
  cdcDeinit,
  cdcSetup,
  cdcDataIn,
  cdcDataOut,
  cdcSof
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void usbSerialPutc(uint8_t c)
{
  /*
   * There is no reliable way to tell whether the virtual serial port has been
   * opened on the host side, so cdcConnected only reports that the physical
   * USB connection is up.  Bytes written before then are simply dropped.
   */

  if (!cdcConnected) {
    return;
  }

  // The ring is shared with the USB interrupt, so the update has to be atomic
  uint32_t prim = __get_PRIMASK();
  __disable_irq();

  if ((APP_Tx_ptr_in + 1) % APP_TX_DATA_SIZE != APP_Rx_ptr_out) {
    serialBuffer.txBuf[APP_Tx_ptr_in] = c;
    APP_Tx_ptr_in = (APP_Tx_ptr_in + 1) % APP_TX_DATA_SIZE;
  }

  if (!prim) {
    __enable_irq();
  }
}
