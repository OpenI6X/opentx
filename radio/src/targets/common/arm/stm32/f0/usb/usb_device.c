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
 * USB device core: endpoint table, packet memory allocator, control transfer
 * state machine, enumeration and the interrupt handler.
 *
 * Replaces ST's usb_core / usb_dcd / usb_dcd_int / usbd_core / usbd_req /
 * usbd_ioreq.  Dropped: double buffering, the suspend/resume state machine,
 * remote wakeup and LPM, none of which this product uses.
 */

#include "stm32_hal_ll.h"    // brings in USE_FULL_LL_DRIVER and the LL headers
#include "usb_device.h"
#include "usb_ll.h"
#include "usb_desc.h"
#include "board.h"
#include <string.h>

#include "stm32f0xx_ll_usb.h"
#include "stm32f0xx_ll_crs.h"

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static UsbEp usbInEp[USB_EP_COUNT];
static UsbEp usbOutEp[USB_EP_COUNT];

static const UsbClass * usbClass = NULL;

static uint8_t usbDevState = USB_STATE_DEFAULT;
static uint8_t usbEp0Stage = USB_EP0_IDLE;
static uint8_t usbSetup[8];

static uint16_t usbCtlSent;      // data bytes sent on the current control IN stage
static uint16_t usbCtlWanted;    // wLength of the current SETUP, for the ZLP rule
static uint8_t  usbPendingAddr;  // deferred SET_ADDRESS, applied after the status stage

// Packet memory allocator.  The buffer descriptor table occupies the first
// bytes of the PMA; the rest is handed out 32 byte aligned, which keeps every
// buffer address a whole number of 16 bit PMA words.
#define USB_PMA_BTABLE_RESERVE  32
static uint16_t usbPmaNext;

static void usbPmaReset(void)
{
  usbPmaNext = USB_PMA_BTABLE_RESERVE;
}

static uint16_t usbPmaAlloc(uint16_t size)
{
  uint16_t addr = (uint16_t)((usbPmaNext + 31U) & ~31U);
  usbPmaNext = (uint16_t)(addr + ((size + 31U) & ~31U));
  return addr;
}

// ---------------------------------------------------------------------------
// Endpoint table
// ---------------------------------------------------------------------------

UsbEp * usbEpGet(uint8_t epNum, bool isIn)
{
  return (epNum < USB_EP_COUNT) ? (isIn ? &usbInEp[epNum] : &usbOutEp[epNum]) : NULL;
}

bool usbIsConfigured(void)
{
  return usbDevState == USB_STATE_CONFIGURED;
}

uint8_t usbEpOpen(uint8_t epNum, bool isIn, uint8_t type, uint8_t maxpkt)
{
  if (epNum >= USB_EP_COUNT) {
    return 0;
  }

  UsbEp * ep = usbEpGet(epNum, isIn);
  ep->buf = NULL;
  ep->len = 0;
  ep->total = 0;
  ep->maxpkt = maxpkt;
  ep->isStall = 0;
  ep->open = 1;

  switch (type) {
    case 1: usbSetEpType(epNum, EP_ISOCHRONOUS); break;
    case 2: usbSetEpType(epNum, EP_BULK); break;
    case 3: usbSetEpType(epNum, EP_INTERRUPT); break;
    default: usbSetEpType(epNum, EP_CONTROL); break;
  }
  usbSetEpAddr(epNum, epNum);

  if (isIn) {
    ep->pma = usbPmaAlloc(maxpkt);
    usbSetTxAddr(epNum, ep->pma);
    usbClearDtoTx(epNum);
    usbSetTxCnt(epNum, 0);
    usbSetTxStatus(epNum, EP_TX_NAK);
  }
  else {
    ep->pma = usbPmaAlloc(maxpkt);
    usbSetRxAddr(epNum, ep->pma);
    usbClearDtoRx(epNum);
    usbSetRxCnt(epNum, maxpkt);
    usbSetRxStatus(epNum, epNum ? EP_RX_NAK : EP_RX_VALID);
  }
  return 1;
}

void usbEpClose(uint8_t epNum, bool isIn)
{
  if (epNum >= USB_EP_COUNT) {
    return;
  }
  UsbEp * ep = usbEpGet(epNum, isIn);
  if (isIn) {
    usbClearDtoTx(epNum);
    usbSetTxStatus(epNum, EP_TX_DIS);
  }
  else {
    usbClearDtoRx(epNum);
    usbSetRxStatus(epNum, EP_RX_DIS);
  }
  ep->open = 0;
  ep->buf = NULL;
  ep->len = 0;
  ep->total = 0;
}

void usbEpStall(uint8_t epNum, bool isIn)
{
  UsbEp * ep = usbEpGet(epNum, isIn);
  if (!ep || !ep->open) {
    return;
  }
  ep->isStall = 1;
  if (isIn) {
    usbSetTxStatus(epNum, EP_TX_STALL);
  }
  else {
    usbSetRxStatus(epNum, EP_RX_STALL);
  }
}

void usbEpClearStall(uint8_t epNum, bool isIn)
{
  UsbEp * ep = usbEpGet(epNum, isIn);
  if (!ep || !ep->open) {
    return;
  }
  ep->isStall = 0;
  // Clearing a halt must also reset the data toggle (USB 2.0 8.5.3.4)
  if (isIn) {
    usbClearDtoTx(epNum);
    usbSetTxStatus(epNum, EP_TX_NAK);
  }
  else {
    usbClearDtoRx(epNum);
    ep0OrEpArmRx(epNum, ep->maxpkt);
  }
}

// ---------------------------------------------------------------------------
// Packet movement
// ---------------------------------------------------------------------------

// Load the next packet of an IN transfer into the PMA and mark it valid.
static void usbEpTxNextPacket(UsbEp * ep, uint8_t epNum)
{
  uint16_t n = (uint16_t)((ep->len > ep->maxpkt) ? ep->maxpkt : ep->len);
  if (n) {
    usbPmaWrite(ep->buf, ep->pma, n);
  }
  usbSetTxCnt(epNum, n);
  // Reset the data toggle before arming.  A SETUP packet makes the hardware
  // restart both directions from DATA0, and ST's driver clears DTOG_TX
  // explicitly before every arm for the same reason; without it the toggle can
  // carry over from the previous transfer and the host sees a PID mismatch.
  usbClearDtoTx(epNum);
  usbSetTxStatus(epNum, EP_TX_VALID);
}

void usbEpStartTx(uint8_t epNum, const uint8_t * buf, uint16_t len)
{
  UsbEp * ep = usbEpGet(epNum, true);
  if (!ep || !ep->open) {
    return;
  }
  ep->buf = (uint8_t *)buf;
  ep->len = len;
  ep->total = len;
  usbEpTxNextPacket(ep, epNum);
}

void usbEpStartRx(uint8_t epNum, uint8_t * buf, uint16_t len)
{
  UsbEp * ep = usbEpGet(epNum, false);
  if (!ep || !ep->open) {
    return;
  }
  ep->buf = buf;
  ep->len = len;
  ep->total = len;
  ep0OrEpArmRx(epNum, (uint16_t)((len > ep->maxpkt) ? ep->maxpkt : len));
}

// Arm an OUT endpoint for the next packet.  EP0 additionally needs the
// STATUS_OUT flag handled, hence the small wrapper.
void ep0OrEpArmRx(uint8_t epNum, uint16_t count)
{
  usbSetRxCnt(epNum, count);
  usbClearDtoRx(epNum);      // see usbEpTxNextPacket: reset the toggle before arming
  usbSetRxStatus(epNum, EP_RX_VALID);
}

// ---------------------------------------------------------------------------
// Control pipe
// ---------------------------------------------------------------------------

static void usbEp0ArmRx(uint16_t count, bool statusOut)
{
  if (statusOut) {
    usbSetStatusOut(0);
  }
  else {
    usbClearStatusOut(0);
  }
  ep0OrEpArmRx(0, count);
}

void usbCtlSendData(const uint8_t * buf, uint16_t len)
{
  usbCtlSent = 0;
  usbCtlWanted = 0;      // no terminating ZLP unless a caller asks for one
  usbEp0Stage = USB_EP0_DATA_IN;
  usbEpStartTx(0, buf, len);
  // The next OUT on EP0 would be a new SETUP, so keep it listening.
  usbEp0ArmRx(USB_EP0_MPS, false);
}

void usbCtlSendStatus(void)
{
  usbCtlSent = 0;
  usbEp0Stage = USB_EP0_STATUS_IN;
  usbEpStartTx(0, NULL, 0);
  usbEp0ArmRx(USB_EP0_MPS, false);
}

void usbCtlError(void)
{
  // STALL IN; the host recovers with CLEAR_FEATURE(ENDPOINT_HALT).
  usbEpStall(0, true);
  usbEp0Stage = USB_EP0_IDLE;
  usbClearStatusOut(0);
  usbEp0ArmRx(USB_EP0_MPS, false);
}

// Arm EP0 to receive a control OUT data stage of len bytes.
void usbCtlPrepareRx(uint8_t * buf, uint16_t len)
{
  usbClearStatusOut(0);
  usbEp0Stage = USB_EP0_DATA_OUT;
  usbEpStartRx(0, buf, len);
}

// ---------------------------------------------------------------------------
// Enumeration
// ---------------------------------------------------------------------------

static void usbStdDevRequest(const UsbSetupReq * req)
{
  switch (req->bRequest) {

    case 0x06: {                                          // GET_DESCRIPTOR
      const uint8_t * buf = NULL;
      uint16_t len = 0;
      switch (req->wValue >> 8) {
        case 0x01:                                        // DEVICE
          buf = usbGetDeviceDesc(&len);
          if (req->wLength == 64) {
            len = 8;                                      // Windows probes with wLength 64
          }
          break;
        case 0x02:                                        // CONFIGURATION
          if (usbClass) {
            buf = usbClass->getConfigDesc(&len);
          }
          break;
        case 0x03:                                        // STRING
          buf = usbGetStringDesc((uint8_t)req->wValue, &len);
          break;
        default:
          // Class specific descriptor types, e.g. the HID report descriptor
          if (usbClass->setup(req)) {
            return;
          }
          break;
      }
      if (buf && len && req->wLength) {
        uint16_t n = (uint16_t)((len < req->wLength) ? len : req->wLength);
        usbCtlSendData(buf, n);
        // If the host asked for more than the descriptor holds and n is a whole
        // number of packets, a zero length packet is needed to end the stage.
        // Safe to set after the call: this runs inside the USB interrupt.
        if (n < req->wLength) {
          usbCtlWanted = req->wLength;
        }
      }
      else {
        usbCtlError();
      }
      break;
    }

    case 0x05:                                            // SET_ADDRESS
      if (req->wIndex == 0 && req->wLength == 0 && usbDevState != USB_STATE_CONFIGURED) {
        usbPendingAddr = (uint8_t)(req->wValue & 0x7F);
        usbCtlSendStatus();
        usbDevState = usbPendingAddr ? USB_STATE_ADDRESSED : USB_STATE_DEFAULT;
      }
      else {
        usbCtlError();
      }
      break;

    case 0x09:                                            // SET_CONFIGURATION
      if (usbDevState == USB_STATE_ADDRESSED || usbDevState == USB_STATE_CONFIGURED) {
        if (!req->wValue) {
          if (usbClass && usbDevState == USB_STATE_CONFIGURED) {
            usbClass->deinit();
          }
          usbDevState = USB_STATE_ADDRESSED;
        }
        else if (usbClass) {
          usbClass->init();
          usbDevState = USB_STATE_CONFIGURED;
        }
        usbCtlSendStatus();
      }
      else {
        usbCtlError();
      }
      break;

    case 0x08: {                                          // GET_CONFIGURATION
      uint8_t cfg = (uint8_t)((usbDevState == USB_STATE_CONFIGURED) ? 1 : 0);
      if (req->wLength != 1) {
        usbCtlError();
      }
      else {
        usbCtlSendData(&cfg, 1);
      }
      break;
    }

    case 0x00: {                                          // GET_STATUS (device)
      uint16_t status = 0;
      if (usbDevState == USB_STATE_DEFAULT) {
        usbCtlError();
      }
      else {
        usbCtlSendData((const uint8_t *)&status, 2);
      }
      break;
    }

    default:
      usbCtlError();
      break;
  }
}

static void usbStdItfRequest(const UsbSetupReq * req)
{
  switch (req->bRequest) {
    case 0x0A: {                                          // GET_INTERFACE
      uint8_t alt = 0;
      usbCtlSendData(&alt, 1);
      break;
    }
    case 0x0B:                                            // SET_INTERFACE
      if (usbClass->setup(req)) {
        return;
      }
      usbCtlSendStatus();
      break;
    case 0x00: {                                          // GET_STATUS (interface)
      uint16_t status = 0;
      usbCtlSendData((const uint8_t *)&status, 2);
      break;
    }
    default:
      if (!usbClass->setup(req)) {
        usbCtlError();
      }
      break;
  }
}

static void usbStdEpRequest(const UsbSetupReq * req)
{
  uint8_t epAddr = (uint8_t)(req->wIndex & 0xFF);
  uint8_t epNum = (uint8_t)(epAddr & 0x7F);
  bool isIn = (epAddr & 0x80) != 0;
  UsbEp * ep = usbEpGet(epNum, isIn);

  if (!ep || !ep->open) {
    usbCtlError();
    return;
  }

  switch (req->bRequest) {
    case 0x00: {                                          // GET_STATUS (endpoint)
      uint16_t status = ep->isStall ? 1 : 0;
      usbCtlSendData((const uint8_t *)&status, 2);
      break;
    }
    case 0x01:                                            // CLEAR_FEATURE
      if (req->wValue != 0x00) {                          // ENDPOINT_HALT
        usbCtlError();
        break;
      }
      // The mass storage class needs to see this to recover from a bad CBW.
      if (usbClass->setup(req)) {
        return;
      }
      usbEpClearStall(epNum, isIn);
      usbCtlSendStatus();
      break;
    case 0x03:                                            // SET_FEATURE
      if (req->wValue != 0x00) {
        usbCtlError();
        break;
      }
      usbEpStall(epNum, isIn);
      usbCtlSendStatus();
      break;
    default:
      usbCtlError();
      break;
  }
}

static void usbHandleSetup(void)
{
  UsbSetupReq req;
  req.bmRequestType = usbSetup[0];
  req.bRequest = usbSetup[1];
  req.wValue = (uint16_t)(usbSetup[2] | (usbSetup[3] << 8));
  req.wIndex = (uint16_t)(usbSetup[4] | (usbSetup[5] << 8));
  req.wLength = (uint16_t)(usbSetup[6] | (usbSetup[7] << 8));

  if (usbDevState == USB_STATE_DEFAULT && req.bRequest != 0x06) {
    // Nothing but GET_DESCRIPTOR is legal before SET_ADDRESS.
    usbCtlError();
    return;
  }

  if (!usbClass) {
    usbCtlError();
    return;
  }

  switch (req.bmRequestType & 0x60) {
    case 0x00:
      // Direction is per request: GET_DESCRIPTOR is IN, but SET_ADDRESS and
      // SET_CONFIGURATION are host to device, so no blanket check here.
      usbStdDevRequest(&req);
      break;

    case 0x01:
      if (usbDevState != USB_STATE_CONFIGURED) {
        usbCtlError();
      }
      else {
        usbStdItfRequest(&req);
      }
      break;

    case 0x02:
      if (usbDevState != USB_STATE_CONFIGURED) {
        usbCtlError();
      }
      else {
        usbStdEpRequest(&req);
      }
      break;

    case 0x20:                                            // class specific
      if (usbDevState == USB_STATE_CONFIGURED && usbClass->setup(&req)) {
        break;
      }
      if (req.wLength == 0) {
        usbCtlSendStatus();
      }
      else {
        usbCtlError();
      }
      break;

    default:
      usbCtlError();
      break;
  }
}

// ---------------------------------------------------------------------------
// Completion handling
// ---------------------------------------------------------------------------

static void usbEp0TxComplete(void)
{
  UsbEp * ep = &usbInEp[0];
  uint16_t done = usbGetTxCnt(0);

  ep->buf += done;
  ep->len = (uint16_t)((ep->len > done) ? ep->len - done : 0);
  usbCtlSent = (uint16_t)(usbCtlSent + done);

  if (usbEp0Stage == USB_EP0_DATA_IN) {
    if (ep->len) {
      usbEpTxNextPacket(ep, 0);
      return;
    }
    // A data stage whose length is an exact multiple of the max packet size
    // is only terminated by a zero length packet, but only if the host is
    // actually expecting more data.
    if (usbCtlSent && ((usbCtlSent % USB_EP0_MPS) == 0) && usbCtlSent < usbCtlWanted) {
      usbCtlWanted = 0;                                   // send the ZLP only once
      usbEpStartTx(0, NULL, 0);
      return;
    }
    // Data stage done: the status stage is an IN transaction.
    usbEp0Stage = USB_EP0_STATUS_IN;
    usbEpStartTx(0, NULL, 0);
    return;
  }

  if (usbEp0Stage == USB_EP0_STATUS_IN) {
    // The device address only takes effect once the status stage is ACKed.
    if (usbPendingAddr) {
      USB_DADDR_R = (uint16_t)(usbPendingAddr | USB_DADDR_EF);
      usbPendingAddr = 0;
    }
    usbEp0Stage = USB_EP0_IDLE;
  }
}

static void usbEp0RxComplete(void)
{
  UsbEp * ep = &usbOutEp[0];
  uint16_t done = usbRxCntBytes(0);
  uint16_t received;

  if (done && ep->buf) {
    usbPmaRead(ep->buf, ep->pma, done);
    ep->buf += done;
  }
  ep->len = (uint16_t)((ep->len > done) ? ep->len - done : 0);
  ep->total = (uint16_t)((ep->total > done) ? ep->total - done : 0);
  received = ep->total;

  if (usbEp0Stage == USB_EP0_DATA_OUT) {
    if (ep->len == 0 || done < ep->maxpkt) {
      // Data stage complete.  Let the class consume it, then the status stage
      // is an OUT transaction from the host.
      if (usbClass) {
        usbClass->dataOut(0, received);
      }
      usbEp0Stage = USB_EP0_STATUS_OUT;
      // The status stage of a control OUT transfer is a single byte from the
      // host (EP_KIND/STATUS_OUT makes the hardware expect exactly that).  A
      // buffer programmed for 0 bytes can never be filled, so the hardware
      // NAKs forever and the host reports the device as not responding.
      usbEp0ArmRx(1, true);
    }
    else {
      usbEp0ArmRx((uint16_t)((ep->len > ep->maxpkt) ? ep->maxpkt : ep->len), false);
    }
    return;
  }

  // A SETUP, or the status stage of a control IN transfer: back to idle.
  usbEp0Stage = USB_EP0_IDLE;
  usbEp0ArmRx(USB_EP0_MPS, false);
}

// The interrupt must never be able to lock up: if a completion arrives that
// this code does not (or cannot) clear, an unbounded drain loop spins here
// forever, the main loop never runs and the watchdog resets the radio.  So the
// drain is bounded.  Anything left over still has its CTR bit set, so the next
// interrupt picks it up - and because each pass does bounded work the main
// loop keeps getting CPU.
#define USB_CTR_MAX_PASSES  32

static void usbCtr(void)
{
  uint16_t istr;
  int guard = USB_CTR_MAX_PASSES;

  while (guard-- > 0 && ((istr = USB_ISTR_R) & USB_ISTR_CTR)) {
    uint8_t epNum = (uint8_t)(istr & USB_ISTR_EP_ID);
    bool isIn = (istr & USB_ISTR_DIR) == 0;
    uint16_t epr;

    if (epNum == 0) {
      if (isIn) {
        usbClearCtrTx(0);
        usbEp0TxComplete();
      }
      else {
        // The SETUP bit is only latched while CTR_RX is set, so it has to be
        // sampled from a snapshot taken before CTR_RX is cleared.
        epr = usbGetEP(0);
        if (epr & EP_SETUP) {
          // A SETUP packet is always exactly 8 bytes.  ST reads a fixed 8
          // rather than trusting the RX count, so do the same: the count field
          // is in whatever unit the hardware last used, and is not a reliable
          // length here.
          usbClearCtrRx(0);
          usbPmaRead(usbSetup, usbOutEp[0].pma, 8);
          usbEp0Stage = USB_EP0_IDLE;
          usbHandleSetup();
        }
        else if (epr & EP_CTR_RX) {
          usbClearCtrRx(0);
          usbEp0RxComplete();
        }
      }
      continue;
    }

    epr = usbGetEP(epNum);

    if (!isIn && (epr & EP_CTR_RX)) {
      UsbEp * ep = &usbOutEp[epNum];
      uint16_t done = usbRxCntBytes(epNum);
      usbClearCtrRx(epNum);
      if (done && ep->buf) {
        usbPmaRead(ep->buf, ep->pma, done);
        ep->buf += done;
      }
      ep->len = (uint16_t)((ep->len > done) ? ep->len - done : 0);
      ep->total = (uint16_t)((ep->total > done) ? ep->total - done : 0);
      if (ep->len == 0 || done < ep->maxpkt) {
        // A short packet terminates an OUT transfer.
        if (usbClass) {
          usbClass->dataOut(epNum, ep->total);
        }
      }
      else {
        ep0OrEpArmRx(epNum, (uint16_t)((ep->len > ep->maxpkt) ? ep->maxpkt : ep->len));
      }
    }

    if (isIn && (epr & EP_CTR_TX)) {
      UsbEp * ep = &usbInEp[epNum];
      uint16_t done = usbGetTxCnt(epNum);
      usbClearCtrTx(epNum);
      ep->buf += done;
      ep->len = (uint16_t)((ep->len > done) ? ep->len - done : 0);
      if (ep->len == 0) {
        if (usbClass) {
          usbClass->dataIn(epNum);
        }
      }
      else {
        usbEpTxNextPacket(ep, epNum);
      }
    }
  }
}

void usbDeviceIsr(void)
{
  uint16_t istr = USB_ISTR_R;

  if (istr & USB_ISTR_RESET) {
    USB_ISTR_R = (uint16_t)USB_CLR_RESET;
    USB_DADDR_R = USB_DADDR_EF;             // back to address 0
    usbPendingAddr = 0;
    usbDevState = USB_STATE_DEFAULT;
    usbEp0Stage = USB_EP0_IDLE;
    memset(usbSetup, 0, sizeof(usbSetup));
    if (usbClass) {
      usbClass->deinit();
    }
    usbEp0ArmRx(USB_EP0_MPS, false);
  }

  if (istr & USB_ISTR_CTR) {
    usbCtr();
  }

  if (istr & USB_ISTR_SOF) {
    USB_ISTR_R = (uint16_t)USB_CLR_SOF;
    if (usbClass) {
      usbClass->sof();
    }
  }

  // Suspend, wakeup, error and ESOF carry no state we act on, so they are not
  // enabled in CNTR (see usbDeviceStart).  Acknowledge anything that is set
  // anyway, so a stray flag can never latch.
  //
  // ISTR is rc_w0: a bit is cleared by writing ZERO to it.  The USB_CLR_xxx
  // macros are the *complement* of each bit, so several of them must be ANDed,
  // not ORed - ORing them yields 0xFFFF and clears nothing at all, which would
  // leave SUSP latched and, with SUSPM enabled, spin the interrupt forever.
  uint16_t ack = (uint16_t)(USB_ISTR_WKUP | USB_ISTR_SUSP | USB_ISTR_ERR |
                            USB_ISTR_ESOF | USB_ISTR_PMAOVR);
  if (istr & ack) {
    USB_ISTR_R = (uint16_t)~ack;
  }
}

// ---------------------------------------------------------------------------
// Bring up / tear down
// ---------------------------------------------------------------------------

static void usbPeripheralInit(void)
{
  USB_CNTR_R = (uint16_t)USB_CNTR_FRES;    // force reset
  USB_CNTR_R = 0;
  USB_ISTR_R = 0;
  USB_BTABLE_R = BTABLE_ADDRESS;
  USB_DADDR_R = USB_DADDR_EF;
}

static void usbEp0Open(void)
{
  usbEpOpen(0, false, 0, USB_EP0_MPS);
  usbEpOpen(0, true, 0, USB_EP0_MPS);
  usbEp0ArmRx(USB_EP0_MPS, false);
}

// USB clock: HSI48 trimmed against the USB SOF, so an accurate 48 MHz without
// a crystal and the rest of the clock tree stays untouched.  This is
// deliberately identical to what the previous stack did in USB_BSP_Init():
// selecting HSI48 as the USB source and starting the CRS auto-trim.  HSI48
// itself is not switched on here - as before, the peripheral is left powered
// down until usbStart().
static void usbClockInit(void)
{
  LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_HSI48);
  LL_CRS_SetSyncSignalSource(LL_CRS_SYNC_SOURCE_USB);
  LL_CRS_EnableAutoTrimming();
  LL_CRS_EnableFreqErrorCounter();
}

void usbDeviceInit(void)
{
  // This runs from boardInit(), so it must do no more than the previous stack
  // did in USB_BSP_Init(): set up the cable detect pin and the 48 MHz clock.
  // The USB registers and the packet memory must NOT be touched here - the
  // peripheral (and therefore the PMA window) is not clocked yet, and writing
  // it before HSI48 is up faults.  All of that is deferred to usbDeviceStart().

  // USB DP/DM double as the cable detect.  While the peripheral owns the pins
  // the pull-up holds D- high, so a 0 on DM means no cable.
  LL_GPIO_InitTypeDef gpioInit = { 0 };
  gpioInit.Pin = USB_GPIO_PIN_DM;
  gpioInit.Mode = LL_GPIO_MODE_INPUT;
  gpioInit.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(USB_GPIO, &gpioInit);

  usbClockInit();

  // Software state only from here on.
  usbPmaReset();
  memset(usbInEp, 0, sizeof(usbInEp));
  memset(usbOutEp, 0, sizeof(usbOutEp));
  usbClass = NULL;
  usbDevState = USB_STATE_DEFAULT;
  usbEp0Stage = USB_EP0_IDLE;
  usbPendingAddr = 0;
  usbCtlSent = 0;
  usbCtlWanted = 0;
}

void usbDeviceStart(const UsbClass * cls)
{
  usbClass = cls;

  usbClockInit();                 // the clock may have been stopped meanwhile
  usbPmaReset();
  memset(usbInEp, 0, sizeof(usbInEp));
  memset(usbOutEp, 0, sizeof(usbOutEp));
  usbDevState = USB_STATE_DEFAULT;
  usbEp0Stage = USB_EP0_IDLE;
  usbPendingAddr = 0;
  usbCtlSent = 0;
  usbCtlWanted = 0;

  usbPeripheralInit();
  usbEp0Open();
  // Only the three events this driver actually services are enabled.  The
  // previous stack also enabled SUSPM/WKUPM/ESOFM/ERRM to drive a resume state
  // machine that no longer exists; leaving them on with nothing to act on is
  // what turns a latched flag into an interrupt storm.
  USB_CNTR_R = (uint16_t)(USB_CNTR_CTRM | USB_CNTR_SOFM | USB_CNTR_RESETM);
  USB_BCDR_R = (uint16_t)(USB_BCDR_R | USB_BCDR_DPPU);     // attach

  NVIC_SetPriority(USB_IRQn, 11);
  NVIC_EnableIRQ(USB_IRQn);
}

void usbDeviceStop(void)
{
  USB_CNTR_R = (uint16_t)USB_CNTR_FRES;
  USB_ISTR_R = 0;
  USB_BCDR_R = (uint16_t)(USB_BCDR_R & ~USB_BCDR_DPPU);   // detach
  USB_CNTR_R = (uint16_t)(USB_CNTR_FRES | USB_CNTR_PDWN);
  NVIC_DisableIRQ(USB_IRQn);
  usbClass = NULL;
  usbDevState = USB_STATE_DEFAULT;
}
