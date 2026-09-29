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
 * USB mass storage class (Bulk Only Transport) with the subset of SCSI that
 * Windows and macOS actually issue.
 *
 * Replaces usbd_msc_core.c, usbd_msc_bot.c, usbd_msc_data.c and
 * usbd_msc_scsi.c.  The ST originals carry MODE_SELECT, READ6/12/16,
 * WRITE6/12/16, VERIFY12/16, SEEK, SYNCHRONIZE CACHE and friends, none of
 * which this virtual disk can service, so they were dropped rather than
 * emulated.
 */

#include "usb_device.h"
#include "usb_msd.h"
#include "usb_storage.h"
#include "board.h"
#include "opentx.h"

#include <string.h>

#define MSC_EP_IN       0x81
#define MSC_EP_OUT      0x02
#define MSC_EP_IN_NUM   1
#define MSC_EP_OUT_NUM  2
#define MSC_MPS         64
#define MSC_MEDIA_PKT   512
// One block per packet: the read and write paths convert bytes to blocks by
// dividing by the block size STORAGE_GetCapacity reports, so these must agree.
static_assert(MSC_MEDIA_PKT == BLOCK_SIZE, "MSC media packet must be one block");

#define CBW_LEN         31
// Command Status Wrapper is 13 bytes on the wire (BOT 6.2.1, ST's
// BOT_CSW_LENGTH).  The struct pads to 16, so the length must stay explicit.
#define CSW_LEN         13

enum {
  BOT_IDLE = 0,
  BOT_DATA_IN,
  BOT_DATA_OUT,
  BOT_LAST_DATA_IN,
  BOT_SEND_DATA
};

enum { CSW_PASSED = 0, CSW_FAILED = 1 };

// SCSI opcodes we support
enum {
  SCSI_TEST_UNIT_READY   = 0x00,
  SCSI_REQUEST_SENSE     = 0x03,
  SCSI_INQUIRY           = 0x12,
  SCSI_MODE_SENSE6       = 0x1A,
  SCSI_ALLOW_MED_REMOVAL = 0x1E,
  SCSI_READ_FORMAT_CAP   = 0x23,
  SCSI_READ_CAPACITY10   = 0x25,
  SCSI_READ10            = 0x28,
  SCSI_WRITE10           = 0x2A,
  SCSI_START_STOP_UNIT   = 0x1B,
  SCSI_MODE_SENSE10      = 0x5A,
  SCSI_VERIFY10          = 0x2F
};

// Sense keys
enum { SENSE_NOT_READY = 2, SENSE_MEDIUM_ERROR = 3, SENSE_ILLEGAL_REQUEST = 5 };
#define ASC_INVALID_CDB          0x20
#define ASC_ADDRESS_OUT_OF_RANGE 0x21
#define ASC_MEDIUM_NOT_PRESENT   0x3A
#define ASC_WRITE_PROTECTED      0x27
#define ASC_UNRECOVERED_READ     0x11
#define ASC_WRITE_FAULT          0x03

typedef struct
{
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataLength;
  uint8_t  bmFlags;
  uint8_t  bLUN;
  uint8_t  bCBLength;
  uint8_t  CB[16];
} MSC_CBW;

typedef struct
{
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataResidue;
  uint8_t  bStatus;
} MSC_CSW;

static const uint8_t mscCfgDesc[32] = {
  0x09, 0x02, 32, 0x00,         // Configuration, total length
  0x01, 0x01, 0x04, 0xC0, 0x32, // 1 interface, value 1, iConfiguration 4, self powered

  0x09, 0x04,                   // Interface
  0x00, 0x00,                   // number, alternate
  0x02,                         // 2 endpoints
  0x08, 0x06, 0x50,             // MSC, SCSI transparent, BOT
  0x05,                         // iInterface

  0x07, 0x05, MSC_EP_IN, 0x02, MSC_MPS, 0x00, 0x00,
  0x07, 0x05, MSC_EP_OUT, 0x02, MSC_MPS, 0x00, 0x00
};

static MSC_CBW cbw;
static MSC_CSW csw;
// The transfer buffer is taken from the shared reusableBuffer union rather
// than a dedicated array: mass storage suspends the menus, so that space is
// already paid for and the baseline stack did exactly this.  Every transfer
// length here is even, so USB_WritePMA()'s round-up to a whole halfword never
// reads past the end.
#define mediaBuf  reusableBuffer.MSC_BOT_Data
static_assert(sizeof(reusableBuffer.MSC_BOT_Data) >= MSC_MEDIA_PKT, "MSC buffer too small");
static uint16_t mediaLen;
static uint8_t botState;

// Sense ring, so consecutive failures are not collapsed
typedef struct
{
  uint8_t key;
  uint8_t asc;
  uint8_t ascq;
} SenseEntry;

#define SENSE_DEPTH 4
static SenseEntry sense[SENSE_DEPTH];
static uint8_t senseHead;
static uint8_t senseTail;

static uint32_t blkAddr;     // current block, for READ10 / WRITE10 bursts
static uint32_t blkLen;      // remaining bytes of the current transfer
static uint8_t  botStatus;   // sticky "bad CBW" state (ST's BOT_STATE_ERROR)
static uint8_t  botRecovery; // set by BOT_RESET, suppresses the FAILED CSW on
                             // the following CLEAR_FEATURE (ST's BOT_STATE_RECOVERY)

static int mscProcessWrite(uint8_t lun);

static void mscSense(uint8_t lun, uint8_t key, uint8_t asc)
{
  (void)lun;
  sense[senseHead].key = key;
  sense[senseHead].asc = asc;
  sense[senseHead].ascq = 0;
  senseHead = (uint8_t)((senseHead + 1) % SENSE_DEPTH);
}

static void mscSendCsw(uint8_t status)
{
  csw.dSignature = 0x53425355;      // "USBS"
  csw.bStatus = status;
  botState = BOT_IDLE;
  usbEpStartTx(MSC_EP_IN_NUM, (const uint8_t *)&csw, CSW_LEN);
  usbEpStartRx(MSC_EP_OUT_NUM, (uint8_t *)&cbw, CBW_LEN);
}

static void mscSendData(const uint8_t * buf, uint16_t len)
{
  if (len > cbw.dDataLength) {
    len = (uint16_t)cbw.dDataLength;
  }
  csw.dDataResidue -= len;
  csw.bStatus = CSW_PASSED;
  botState = BOT_SEND_DATA;
  usbEpStartTx(MSC_EP_IN_NUM, buf, len);
}

static void mscAbort(void)
{
  if (cbw.bmFlags == 0 && cbw.dDataLength != 0 && botStatus == 0) {
    usbEpStall(MSC_EP_OUT_NUM, false);
  }
  usbEpStall(MSC_EP_IN_NUM, true);
  if (botStatus != 0) {
    usbEpStartRx(MSC_EP_OUT_NUM, (uint8_t *)&cbw, CBW_LEN);
  }
}

// ---------------------------------------------------------------------------
// SCSI
// ---------------------------------------------------------------------------

// Copy n bytes out of a fixed response table into the media buffer.  The BOT
// layer performs the actual IN transfer, so handlers only ever stage data.
static void mscStageConst(const uint8_t * src, uint16_t n, uint16_t max)
{
  if (n > max) {
    n = max;
  }
  memcpy(mediaBuf, src, n);
  mediaLen = n;
}

static int mscTestUnitReady(uint8_t lun)
{
  if (cbw.dDataLength != 0) {
    mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_INVALID_CDB);
    return -1;
  }
  if (STORAGE_IsReady(lun) != 0) {
    mscSense(lun, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
    return -1;
  }
  mediaLen = 0;
  return 0;
}

static int mscRequestSense(uint8_t lun, const uint8_t * cb)
{
  (void)lun;
  memset(mediaBuf, 0, 18);
  mediaBuf[0] = 0x70;                    // current error, fixed format
  mediaBuf[7] = 12;                      // additional sense length
  if (senseHead != senseTail) {
    // Fixed format sense data: byte 2 is the sense key, byte 12 the ASC and
    // byte 13 the ASCQ.  Dequeue from the tail so consecutive failures are
    // reported in order.
    mediaBuf[2] = sense[senseTail].key;
    mediaBuf[12] = sense[senseTail].asc;
    mediaBuf[13] = sense[senseTail].ascq;
    senseTail = (uint8_t)((senseTail + 1) % SENSE_DEPTH);
  }
  mediaLen = 18;
  if (cb[4] <= 18) {
    mediaLen = cb[4];
  }
  return 0;
}

static int mscInquiry(uint8_t lun, const uint8_t * cb)
{
  if (cb[1] & 0x01) {                    // EVPD: vital product data page 00
    static const uint8_t page00[7] = { 0x00, 0x00, 0x00, 0x03, 0x00, 0x80, 0x83 };
    mscStageConst(page00, sizeof(page00), 0xFF);
  }
  else {
    const uint8_t * p = &STORAGE_Inquirydata[lun * 36];
    uint16_t len = (uint16_t)(p[4] + 5);
    mscStageConst(p, len, (cb[4] < len) ? cb[4] : len);
  }
  return 0;
}

static int mscReadCapacity10(uint8_t lun, const uint8_t * cb)
{
  uint32_t nbr, size;
  (void)cb;
  if (STORAGE_GetCapacity(lun, &nbr, &size) != 0) {
    mscSense(lun, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
    return -1;
  }
  mediaBuf[0] = (uint8_t)((nbr - 1) >> 24);
  mediaBuf[1] = (uint8_t)((nbr - 1) >> 16);
  mediaBuf[2] = (uint8_t)((nbr - 1) >> 8);
  mediaBuf[3] = (uint8_t)(nbr - 1);
  mediaBuf[4] = (uint8_t)(size >> 24);
  mediaBuf[5] = (uint8_t)(size >> 16);
  mediaBuf[6] = (uint8_t)(size >> 8);
  mediaBuf[7] = (uint8_t)size;
  mediaLen = 8;
  return 0;
}

static int mscReadFormatCapacity(uint8_t lun, const uint8_t * cb)
{
  uint32_t nbr, size;
  (void)cb;
  if (STORAGE_GetCapacity(lun, &nbr, &size) != 0) {
    mscSense(lun, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
    return -1;
  }
  mediaBuf[0] = 0;
  mediaBuf[1] = 0;
  mediaBuf[2] = 0;
  mediaBuf[3] = 0x08;
  mediaBuf[4] = (uint8_t)((nbr - 1) >> 24);
  mediaBuf[5] = (uint8_t)((nbr - 1) >> 16);
  mediaBuf[6] = (uint8_t)((nbr - 1) >> 8);
  mediaBuf[7] = (uint8_t)(nbr - 1);
  mediaBuf[8] = 0x02;
  mediaBuf[9] = (uint8_t)(size >> 16);
  mediaBuf[10] = (uint8_t)(size >> 8);
  mediaBuf[11] = (uint8_t)size;
  mediaLen = 12;
  return 0;
}

static int mscModeSense(uint8_t lun, bool ten)
{
  static const uint8_t mode6[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  static const uint8_t mode10[8] = { 0, 6, 0, 0, 0, 0, 0, 0 };
  (void)lun;
  memcpy(mediaBuf, ten ? mode10 : mode6, 8);
  mediaLen = 8;
  return 0;
}

// One 512 byte block of a READ10 burst
static int mscProcessRead(uint8_t lun)
{
  uint32_t nbr, size;
  uint32_t len;

  if (STORAGE_GetCapacity(lun, &nbr, &size) != 0) {
    mscSense(lun, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
    return -1;
  }

  len = (blkLen < MSC_MEDIA_PKT) ? blkLen : MSC_MEDIA_PKT;
  if (STORAGE_Read(lun, mediaBuf, blkAddr, (uint16_t)(len / size)) < 0) {
    mscSense(lun, SENSE_MEDIUM_ERROR, ASC_UNRECOVERED_READ);
    return -1;
  }

  usbEpStartTx(MSC_EP_IN_NUM, mediaBuf, (uint16_t)len);
  blkAddr += len / size;
  blkLen -= len;
  csw.dDataResidue -= len;
  if (blkLen == 0) {
    botState = BOT_LAST_DATA_IN;
  }
  return 0;
}

static int mscRead10(uint8_t lun, const uint8_t * cb)
{
  if (botState == BOT_IDLE) {
    if (cbw.bmFlags != 0x80) {
      mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_INVALID_CDB);
      return -1;
    }
    if (STORAGE_IsReady(lun) != 0) {
      mscSense(lun, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
      return -1;
    }

    blkAddr = ((uint32_t)cb[2] << 24) | ((uint32_t)cb[3] << 16) |
              ((uint32_t)cb[4] << 8) | cb[5];
    blkLen = (uint32_t)(((cb[7] << 8) | cb[8]));

    uint32_t nbr, size;
    if (STORAGE_GetCapacity(lun, &nbr, &size) != 0) {
      mscSense(lun, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
      return -1;
    }
    if (blkAddr + blkLen > nbr) {
      mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_ADDRESS_OUT_OF_RANGE);
      return -1;
    }

    blkLen *= size;
    if (cbw.dDataLength != blkLen) {
      mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_INVALID_CDB);
      return -1;
    }
    botState = BOT_DATA_IN;
  }

  mediaLen = MSC_MEDIA_PKT;
  return mscProcessRead(lun);
}

static int mscWrite10(uint8_t lun, const uint8_t * cb)
{
  uint32_t nbr, size;

  if (botState == BOT_IDLE) {
    if (cbw.bmFlags == 0x80) {
      mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_INVALID_CDB);
      return -1;
    }
    if (STORAGE_IsReady(lun) != 0) {
      mscSense(lun, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
      return -1;
    }
    if (STORAGE_IsWriteProtected(lun) != 0) {
      mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_WRITE_PROTECTED);
      return -1;
    }

    blkAddr = ((uint32_t)cb[2] << 24) | ((uint32_t)cb[3] << 16) |
              ((uint32_t)cb[4] << 8) | cb[5];
    blkLen = (uint32_t)(((cb[7] << 8) | cb[8]));

    if (STORAGE_GetCapacity(lun, &nbr, &size) != 0) {
      mscSense(lun, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
      return -1;
    }
    if (blkAddr + blkLen > nbr) {
      mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_ADDRESS_OUT_OF_RANGE);
      return -1;
    }

    blkLen *= size;
    if (cbw.dDataLength != blkLen) {
      mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_INVALID_CDB);
      return -1;
    }

    // Arm for the first data-out packet; the write itself happens when the
    // packet arrives, not now.
    mediaLen = 0;
    botState = BOT_DATA_OUT;
    usbEpStartRx(MSC_EP_OUT_NUM, mediaBuf,
                 (blkLen < MSC_MEDIA_PKT) ? (uint16_t)blkLen : MSC_MEDIA_PKT);
    return 0;
  }

  return mscProcessWrite(lun);
}

// One 512 byte block of a WRITE10 burst
static int mscProcessWrite(uint8_t lun)
{
  uint32_t nbr, size;
  uint32_t len;

  if (STORAGE_GetCapacity(lun, &nbr, &size) != 0) {
    mscSense(lun, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
    return -1;
  }

  len = (blkLen < MSC_MEDIA_PKT) ? blkLen : MSC_MEDIA_PKT;
  if (STORAGE_Write(lun, mediaBuf, blkAddr, (uint16_t)(len / size)) < 0) {
    mscSense(lun, SENSE_MEDIUM_ERROR, ASC_WRITE_FAULT);
    return -1;
  }

  blkAddr += len / size;
  blkLen -= len;
  csw.dDataResidue -= len;

  if (blkLen == 0) {
    mscSendCsw(CSW_PASSED);
  }
  else {
    usbEpStartRx(MSC_EP_OUT_NUM, mediaBuf, (blkLen < MSC_MEDIA_PKT) ? (uint16_t)blkLen : MSC_MEDIA_PKT);
  }
  return 0;
}

static int mscVerify10(uint8_t lun, const uint8_t * cb)
{
  uint32_t nbr, size;
  if (cb[1] & 0x02) {                    // BYTCHK: we cannot verify data
    mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_INVALID_CDB);
    return -1;
  }
  blkAddr = ((uint32_t)cb[2] << 24) | ((uint32_t)cb[3] << 16) |
            ((uint32_t)cb[4] << 8) | cb[5];
  blkLen = (uint32_t)(((cb[7] << 8) | cb[8]));
  if (STORAGE_GetCapacity(lun, &nbr, &size) != 0) {
    mscSense(lun, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
    return -1;
  }
  if (blkAddr + blkLen > nbr) {
    mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_ADDRESS_OUT_OF_RANGE);
    return -1;
  }
  mediaLen = 0;
  return 0;
}

static int mscProcessCmd(uint8_t lun, const uint8_t * cb)
{
  switch (cb[0]) {
    case SCSI_TEST_UNIT_READY:   return mscTestUnitReady(lun);
    case SCSI_REQUEST_SENSE:     return mscRequestSense(lun, cb);
    case SCSI_INQUIRY:           return mscInquiry(lun, cb);
    case SCSI_START_STOP_UNIT:   mediaLen = 0; return 0;
    case SCSI_ALLOW_MED_REMOVAL: mediaLen = 0; return 0;
    case SCSI_MODE_SENSE6:       return mscModeSense(lun, false);
    case SCSI_MODE_SENSE10:      return mscModeSense(lun, true);
    case SCSI_READ_FORMAT_CAP:   return mscReadFormatCapacity(lun, cb);
    case SCSI_READ_CAPACITY10:   return mscReadCapacity10(lun, cb);
    case SCSI_READ10:            return mscRead10(lun, cb);
    case SCSI_WRITE10:           return mscWrite10(lun, cb);
    case SCSI_VERIFY10:          return mscVerify10(lun, cb);
    default:
      mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_INVALID_CDB);
      return -1;
  }
}

// ---------------------------------------------------------------------------
// BOT
// ---------------------------------------------------------------------------

static void mscDecodeCbw(uint8_t lun)
{
  csw.dTag = cbw.dTag;
  csw.dDataResidue = cbw.dDataLength;
  botRecovery = 0;

  if (cbw.dSignature != 0x43425355 ||     // "USBC"
      cbw.bCBLength < 1 || cbw.bCBLength > 16 ||
      cbw.bLUN > (uint8_t)STORAGE_GetMaxLun())
  {
    // Bad CBW (ST's BOT_STATE_ERROR): stall both endpoints and wait for
    // BOT_RESET.  Like ST's MSC_BOT_Abort, the OUT endpoint is re-armed so
    // the next CBW can still arrive.
    mscSense(lun, SENSE_ILLEGAL_REQUEST, ASC_INVALID_CDB);
    botStatus = 1;
    mscAbort();
    return;
  }

  if (mscProcessCmd(lun, cbw.CB) < 0) {
    mscAbort();
    return;
  }

  // A burst transfer has already armed the endpoint itself
  if (botState != BOT_DATA_IN && botState != BOT_DATA_OUT && botState != BOT_LAST_DATA_IN) {
    if (mediaLen > 0) {
      mscSendData(mediaBuf, mediaLen);
    }
    else {
      mscSendCsw(CSW_PASSED);
    }
  }
}

// ---------------------------------------------------------------------------
// Class callbacks
// ---------------------------------------------------------------------------

static const uint8_t * mscGetConfigDesc(uint16_t * len)
{
  *len = sizeof(mscCfgDesc);
  return mscCfgDesc;
}

static void mscInit(void)
{
  botState = BOT_IDLE;
  botStatus = 0;
  botRecovery = 0;
  senseHead = 0;
  senseTail = 0;
  csw.dDataResidue = 0;

  usbEpOpen(MSC_EP_IN_NUM, true, 2 /* bulk */, MSC_MPS);
  usbEpOpen(MSC_EP_OUT_NUM, false, 2 /* bulk */, MSC_MPS);
  usbEpStartRx(MSC_EP_OUT_NUM, (uint8_t *)&cbw, CBW_LEN);
}

static void mscDeinit(void)
{
  usbEpClose(MSC_EP_IN_NUM, true);
  usbEpClose(MSC_EP_OUT_NUM, false);
  botState = BOT_IDLE;
}

static bool mscSetup(const UsbSetupReq * req)
{
  if (req->bmRequestType & 0x20) {
    switch (req->bRequest) {
      case 0xFE: {                                  // BOT_GET_MAX_LUN
        uint8_t maxLun = (uint8_t)STORAGE_GetMaxLun();
        // A single LUN device must not answer this request (BOT 6.3.2)
        if (req->wValue == 0 && req->wLength == 1 && (req->bmRequestType & 0x80) && maxLun > 0) {
          usbCtlSendData(&maxLun, 1);
          return true;
        }
        return false;
      }
      case 0xFF:                                    // BOT_RESET
        if (req->wValue == 0 && req->wLength == 0 && !(req->bmRequestType & 0x80)) {
          // Like ST's MSC_BOT_Reset: back to IDLE in recovery state and
          // listening for the next CBW.  The recovery flag suppresses the
          // FAILED CSW if the host clears a halt straight after the reset.
          botState = BOT_IDLE;
          botStatus = 0;
          botRecovery = 1;
          usbEpStartRx(MSC_EP_OUT_NUM, (uint8_t *)&cbw, CBW_LEN);
          usbCtlSendStatus();
          return true;
        }
        return false;
      default:
        return false;
    }
  }

  // CLEAR_FEATURE(ENDPOINT_HALT) recovery, like ST's MSC_BOT_CplClrFeature.
  // NB: the core clears the stall first and sends the EP0 status after this
  // returns, so queuing the CSW here lands on a NAKed endpoint as intended.
  // A failed command (stall, no CSW yet) completes here with CSW FAILED;
  // without it the host waits for the CSW forever after any SCSI error.
  if (req->bRequest == 0x01 && req->wValue == 0x00) {
    uint8_t clearedNum = (uint8_t)(req->wIndex & 0x7F);
    bool clearedIn = (req->wIndex & 0x80) != 0;
    if (clearedNum != 0 && botStatus != 0) {
      // Bad CBW: stay stalled until BOT_RESET, back to normal state.
      usbEpStall(MSC_EP_IN_NUM, true);
      botStatus = 0;
    }
    else if (clearedNum != 0 && clearedIn && !botRecovery) {
      mscSendCsw(CSW_FAILED);
    }
  }
  return false;                                     // let the core handle it
}

static void mscDataIn(uint8_t ep)
{
  switch (botState) {
    case BOT_DATA_IN:
      // next chunk of a read burst
      if (mscProcessCmd(cbw.bLUN, cbw.CB) < 0) {
        mscSendCsw(CSW_FAILED);
      }
      break;
    case BOT_SEND_DATA:
    case BOT_LAST_DATA_IN:
      mscSendCsw(CSW_PASSED);
      break;
    default:
      // the completion of the CSW itself, or of the notification endpoint
      break;
  }
}

static void mscDataOut(uint8_t ep, uint16_t len)
{
  (void)len;
  if (botState == BOT_IDLE) {
    mscDecodeCbw(cbw.bLUN);
  }
  else if (botState == BOT_DATA_OUT) {
    if (mscProcessCmd(cbw.bLUN, cbw.CB) < 0) {
      mscSendCsw(CSW_FAILED);
    }
  }
}

static void mscSof(void)
{
}

const UsbClass usbMscClass = {
  mscGetConfigDesc,
  mscInit,
  mscDeinit,
  mscSetup,
  mscDataIn,
  mscDataOut,
  mscSof
};
