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
 * Compact low level access to the STM32F0x2 USB full speed device peripheral.
 *
 * The F0 USB core has no "easy mode": every write to an EPnR register must
 * preserve the DTOG toggle bits, and the RX count field in the buffer table is
 * a *block descriptor*, not a byte count.  The macros below encode the correct
 * idiom for both.
 */

#ifndef OPENTX_STM32F0_USB_LL_H
#define OPENTX_STM32F0_USB_LL_H

#include <stdint.h>
#include "stm32f0xx.h"

#ifdef __cplusplus
extern "C" {
#endif

// The LL USB driver (stm32f0xx_ll_usb.c) needs these, but the HAL headers
// leave them commented out.  Provide them once, centrally.
#ifndef EP_TYPE_CTRL
#define EP_TYPE_CTRL   0U
#endif
#ifndef BTABLE_ADDRESS
#define BTABLE_ADDRESS 0x000U
#endif

#define PMA_ACCESS     1U
#define PMA_BASE       0x400U   // offset of the packet memory inside the USB peripheral window

// EPnR bit fields.  EPREG_MASK deliberately excludes the two DTOG bits: any
// write that includes them would toggle the data toggle, so it must never be
// echoed back.
#define EP_CTR_RX      0x8000u
#define EP_DTOG_RX     0x4000u
#define EP_SETUP       0x0800u
#define EP_T_FIELD     0x0600u
#define EP_KIND        0x0100u
#define EP_CTR_TX      0x0080u
#define EP_DTOG_TX     0x0040u
#define EPADDR_FIELD   0x000Fu
#define EPREG_MASK     (EP_CTR_RX | EP_SETUP | EP_T_FIELD | EP_KIND | EP_CTR_TX | EPADDR_FIELD)

// STAT_TX[1:0] encoding: writing 0 to a DTOG bit has no effect, writing 1 flips it
#define EP_TX_DIS      0x0000u
#define EP_TX_STALL    0x0010u
#define EP_TX_NAK      0x0020u
#define EP_TX_VALID    0x0030u
#define EPTX_DTOG1     0x0010u
#define EPTX_DTOG2     0x0020u

// STAT_RX[1:0]
#define EP_RX_DIS      0x0000u
#define EP_RX_STALL    0x1000u
#define EP_RX_NAK      0x2000u
#define EP_RX_VALID    0x3000u
#define EPRX_DTOG1     0x1000u
#define EPRX_DTOG2     0x2000u
#define EPRX_STAT      0x3000u
#define EPTX_STAT      0x0030u
// Preserve everything the hardware owns except the DTOG bits, which are
// written as a transition rather than a value.
#define EPRX_DTOGMASK  (EPRX_STAT | EPREG_MASK)
#define EPTX_DTOGMASK  (EPTX_STAT | EPREG_MASK)
#define EPKIND_MASK    ((uint16_t)(~EP_KIND & EPREG_MASK))

// EPnR type field values
#define EP_CONTROL     0x0200u
#define EP_ISOCHRONOUS 0x0400u
#define EP_BULK        0x0000u
#define EP_INTERRUPT   0x0600u

// ---------------------------------------------------------------------------
// Direct register access
// ---------------------------------------------------------------------------

#define USB_EP_REG(ep)  (*(__IO uint16_t *)((uint32_t)USB + (uint32_t)(ep) * 4U))
#define USB_CNTR_R     (*(__IO uint16_t *)(USB_CNTR))
#define USB_ISTR_R     (*(__IO uint16_t *)(USB_ISTR))
#define USB_DADDR_R    (*(__IO uint16_t *)(USB_DADDR))
#define USB_BTABLE_R   (*(__IO uint16_t *)(USB_BTABLE))
#define USB_BCDR_R     (*(__IO uint16_t *)(USB_BCDR))

static inline uint16_t usbGetEP(uint8_t ep)
{
  return USB_EP_REG(ep);
}

static inline void usbSetEP(uint8_t ep, uint16_t val)
{
  USB_EP_REG(ep) = val;
}

// ---------------------------------------------------------------------------
// Buffer descriptor table (BTABLE)
//
// Each endpoint owns an 8 byte row: TX address, TX count, RX address, RX count.
// BTABLE holds the PMA byte offset of row 0.
// ---------------------------------------------------------------------------

static inline __IO uint16_t * usbBT(uint8_t ep, uint8_t slot)
{
  return (__IO uint16_t *)((uint32_t)USB + USB_BTABLE_R + PMA_BASE + ((ep) * 8U + (slot)) * PMA_ACCESS);
}

static inline void usbSetTxAddr(uint8_t ep, uint16_t addr)   { *usbBT(ep, 0) = (uint16_t)((addr >> 1) << 1); }
static inline void usbSetTxCnt(uint8_t ep, uint16_t len)     { *usbBT(ep, 1) = len; }
static inline void usbSetRxAddr(uint8_t ep, uint16_t addr)   { *usbBT(ep, 2) = (uint16_t)((addr >> 1) << 1); }

// Encode an RX byte count into the block descriptor format.  A count of 64
// needs 32 byte blocks, and a 32 byte block buffer must be 32 byte aligned,
// which is why the PMA allocator hands out aligned buffers.
static inline void usbSetRxCnt(uint8_t ep, uint16_t len)
{
  __IO uint16_t * p = usbBT(ep, 3);
  uint16_t blocks;
  if (len > 62) {
    blocks = (uint16_t)(len >> 5);
    if ((len & 0x1F) == 0) {
      blocks--;                      // NUM_BLOCK is n-1; 0 is reserved for a ZLP
    }
    *p = (uint16_t)((blocks << 10) | 0x8000);
  }
  else {
    blocks = (uint16_t)(len >> 1);
    if ((len & 1) != 0) {
      blocks++;                      // round up to whole 2 byte blocks
    }
    *p = (uint16_t)(blocks << 10);
  }
}

// Decode the block descriptor back into a byte count.  Both ST libraries
// simply mask with 0x3ff and never invert the block encoding, which is wrong;
// this does the inverse properly.
// Note the asymmetry, which is a property of the hardware encoding rather than
// of this code: in 2 byte block mode NUM_BLOCK is the block count itself, while
// in 32 byte block mode it is the count minus one (0 being reserved for a zero
// length packet).  Both ST libraries get this wrong by masking with 0x3ff and
// never inverting the encoding at all.
static inline uint16_t usbRxCntBytes(uint8_t ep)
{
  uint16_t reg = *usbBT(ep, 3);
  uint16_t blocks = (uint16_t)((reg >> 10) & 0x1F);
  if (blocks == 0) {
    return 0;                            // zero length packet
  }
  if (reg & 0x8000) {
    return (uint16_t)((blocks + 1) << 5);
  }
  return (uint16_t)(blocks << 1);
}

static inline uint16_t usbGetTxCnt(uint8_t ep)
{
  return (uint16_t)(*usbBT(ep, 1) & 0x3FF);
}

// ---------------------------------------------------------------------------
// Endpoint status transitions
//
// The DTOG bits are "write 1 to toggle".  So to move an endpoint to a new
// state we must express the transition as the DTOG bits that need flipping.
// ---------------------------------------------------------------------------

static inline void usbSetTxStatus(uint8_t ep, uint16_t state)
{
  uint16_t v = usbGetEP(ep) & EPTX_DTOGMASK;
  if (state & EPTX_DTOG1) v ^= EPTX_DTOG1;
  if (state & EPTX_DTOG2) v ^= EPTX_DTOG2;
  usbSetEP(ep, v | EP_CTR_RX | EP_CTR_TX);
}

static inline void usbSetRxStatus(uint8_t ep, uint16_t state)
{
  uint16_t v = usbGetEP(ep) & EPRX_DTOGMASK;
  if (state & EPRX_DTOG1) v ^= EPRX_DTOG1;
  if (state & EPRX_DTOG2) v ^= EPRX_DTOG2;
  usbSetEP(ep, v | EP_CTR_RX | EP_CTR_TX);
}

static inline void usbClearCtrRx(uint8_t ep) { usbSetEP(ep, (uint16_t)(usbGetEP(ep) & 0x7FFF) & EPREG_MASK); }
static inline void usbClearCtrTx(uint8_t ep) { usbSetEP(ep, (uint16_t)(usbGetEP(ep) & 0xFF7F) & EPREG_MASK); }

static inline void usbClearDtoRx(uint8_t ep)
{
  if (usbGetEP(ep) & EP_DTOG_RX) {
    usbSetEP(ep, EP_CTR_RX | EP_CTR_TX | EP_DTOG_RX | (usbGetEP(ep) & EPREG_MASK));
  }
}

static inline void usbClearDtoTx(uint8_t ep)
{
  if (usbGetEP(ep) & EP_DTOG_TX) {
    usbSetEP(ep, EP_CTR_RX | EP_CTR_TX | EP_DTOG_TX | (usbGetEP(ep) & EPREG_MASK));
  }
}

// EP_KIND is triple purpose: STATUS_OUT on a control endpoint.  We never use
// double buffering, so this is only ever the control-endpoint meaning.
static inline void usbSetStatusOut(uint8_t ep)
{
  usbSetEP(ep, EP_CTR_RX | EP_CTR_TX | ((usbGetEP(ep) | EP_KIND) & EPREG_MASK));
}

static inline void usbClearStatusOut(uint8_t ep)
{
  usbSetEP(ep, EP_CTR_RX | EP_CTR_TX | (usbGetEP(ep) & EPKIND_MASK));
}

static inline void usbSetEpType(uint8_t ep, uint16_t type)
{
  usbSetEP(ep, (uint16_t)((usbGetEP(ep) & EPREG_MASK & ~EP_T_FIELD) | type) | EP_CTR_RX | EP_CTR_TX);
}

static inline void usbSetEpAddr(uint8_t ep, uint8_t addr)
{
  usbSetEP(ep, EP_CTR_RX | EP_CTR_TX | (usbGetEP(ep) & EPREG_MASK) | addr);
}

// ---------------------------------------------------------------------------
// Packet memory accessors
//
// Delegated to the ST LL driver, which handles the 16 bit wide / little endian
// PMA window correctly.  Note USB_WritePMA() rounds an odd length up to a whole
// halfword, so any source buffer handed to it must have one byte of slack.
// ---------------------------------------------------------------------------

void usbPmaWrite(const uint8_t * src, uint16_t pmaAddr, uint16_t len);
void usbPmaRead(uint8_t * dst, uint16_t pmaAddr, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // OPENTX_STM32F0_USB_LL_H
