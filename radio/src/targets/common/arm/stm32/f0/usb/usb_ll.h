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

// Note the *2 on the slot index: each slot is a 16-bit word, so consecutive
// slots are 2 bytes apart.  PMA_ACCESS scales the whole offset and is 1 for
// the 16-bit access mode, so it must not be used for the slot stride.  Without
// the *2 the odd slots land on odd byte offsets, and a halfword store to an
// odd PMA address is a misaligned bus access that HardFaults.
static inline __IO uint16_t * usbBT(uint8_t ep, uint8_t slot)
{
  return (__IO uint16_t *)((uint32_t)USB + USB_BTABLE_R + PMA_BASE + ((ep) * 8U + (slot) * 2U) * PMA_ACCESS);
}

static inline void usbSetTxAddr(uint8_t ep, uint16_t addr)   { *usbBT(ep, 0) = (uint16_t)((addr >> 1) << 1); }
static inline void usbSetTxCnt(uint8_t ep, uint16_t len)     { *usbBT(ep, 1) = len; }
static inline void usbSetRxAddr(uint8_t ep, uint16_t addr)   { *usbBT(ep, 2) = (uint16_t)((addr >> 1) << 1); }

// RX count register.
//
// This deliberately uses ST's convention - a raw byte count in COUNT_RX[9:0],
// with bit 15 left clear (2 byte block mode) - rather than the 32 byte block
// mode encoding.  ST's device library programs EP0 with a raw byte count and
// reads it back with a plain mask, and that path is known to enumerate on this
// part, so matching it exactly is the safe choice.  The 32 byte block mode
// needs the buffer 32 byte aligned and has an asymmetric encode/decode (the
// received value is a block count, not a length), which is easy to get wrong
// and cannot be validated without hardware.
//
// The allocator hands out 32 byte aligned buffers, which satisfies the weaker
// 2 byte alignment this mode needs.
static inline void usbSetRxCnt(uint8_t ep, uint16_t len)
{
  *usbBT(ep, 3) = (uint16_t)(len & 0x3FF);
}

static inline uint16_t usbRxCntBytes(uint8_t ep)
{
  return (uint16_t)(*usbBT(ep, 3) & 0x3FF);
}

static inline uint16_t usbGetTxCnt(uint8_t ep)
{
  return (uint16_t)(*usbBT(ep, 1) & 0x3FF);
}

// ---------------------------------------------------------------------------
// EPnR_KEEP is EPREG_MASK plus both STAT fields, so a read-modify-write of
// EPnR leaves the endpoint's direction status alone.  It excludes the two DTOG
// bits (write-1-to-toggle, never echo them back) and the two CTR bits
// (write-1-to-clear, handled explicitly).
//
// EPREG_MASK, as ST defines it, does not include the STAT fields.  Unlike the
// DTOG bits they are ordinary latched state, and writing 0 to them means
// DISABLE, so a read-modify-write using EPREG_MASK alone switches off whichever
// direction it was not changing - for instance arming EP0 RX would cancel an
// EP0 TX that had just been armed.
#define EPnR_KEEP        (uint16_t)(EPREG_MASK | EPRX_STAT | EPTX_STAT)

// Endpoint status transitions.
//
// STAT_RX/STAT_TX can be written directly, provided the corresponding DTOG bit
// is written as 0 (no toggle).  EPnR_KEEP excludes both DTOG bits, so every
// write below leaves them clear, which is what makes these helpers idempotent:
// asking for VALID twice writes VALID twice rather than toggling back to
// DISABLE as a DTOG based transition would.
// ---------------------------------------------------------------------------

static inline void usbSetTxStatus(uint8_t ep, uint16_t state)
{
  uint16_t v = (uint16_t)(usbGetEP(ep) & EPnR_KEEP & ~EPTX_STAT) | (state & EPTX_STAT);
  usbSetEP(ep, v | EP_CTR_RX | EP_CTR_TX);
}

static inline void usbSetRxStatus(uint8_t ep, uint16_t state)
{
  uint16_t v = (uint16_t)(usbGetEP(ep) & EPnR_KEEP & ~EPRX_STAT) | (state & EPRX_STAT);
  usbSetEP(ep, v | EP_CTR_RX | EP_CTR_TX);
}

static inline void usbClearCtrRx(uint8_t ep) { usbSetEP(ep, (uint16_t)(usbGetEP(ep) & 0x7FFF) & EPnR_KEEP); }
static inline void usbClearCtrTx(uint8_t ep) { usbSetEP(ep, (uint16_t)(usbGetEP(ep) & 0xFF7F) & EPnR_KEEP); }

static inline void usbClearDtoRx(uint8_t ep)
{
  if (usbGetEP(ep) & EP_DTOG_RX) {
    usbSetEP(ep, EP_CTR_RX | EP_CTR_TX | EP_DTOG_RX | (usbGetEP(ep) & EPnR_KEEP));
  }
}

static inline void usbClearDtoTx(uint8_t ep)
{
  if (usbGetEP(ep) & EP_DTOG_TX) {
    usbSetEP(ep, EP_CTR_RX | EP_CTR_TX | EP_DTOG_TX | (usbGetEP(ep) & EPnR_KEEP));
  }
}

// EP_KIND is triple purpose: STATUS_OUT on a control endpoint.  We never use
// double buffering, so this is only ever the control-endpoint meaning.
static inline void usbSetStatusOut(uint8_t ep)
{
  usbSetEP(ep, EP_CTR_RX | EP_CTR_TX | ((usbGetEP(ep) | EP_KIND) & EPnR_KEEP));
}

static inline void usbClearStatusOut(uint8_t ep)
{
  usbSetEP(ep, EP_CTR_RX | EP_CTR_TX | (usbGetEP(ep) & EPnR_KEEP & ~(uint16_t)EP_KIND));
}

static inline void usbSetEpType(uint8_t ep, uint16_t type)
{
  usbSetEP(ep, (uint16_t)((usbGetEP(ep) & EPnR_KEEP & ~EP_T_FIELD) | type) | EP_CTR_RX | EP_CTR_TX);
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
