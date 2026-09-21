/*
 * Copyright (C) OpenI6X
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
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

#ifndef _SERIAL_BUFFER_UNION_H_
#define _SERIAL_BUFFER_UNION_H_

#include "fifo.h"
#include "usbd_conf.h"

#define AUX_SERIAL_TX_FIFO_SIZE 128

union SerialBufferUnion {
  uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];
  Fifo<uint8_t, AUX_SERIAL_TX_FIFO_SIZE> auxSerialTxFifo;
  // Fifo has a user-provided constructor, so without this the
  // union's implicit default constructor would be deleted.
  SerialBufferUnion() : auxSerialTxFifo() {}
};

#ifdef __cplusplus

// C linkage: usbd_cdc_core.c (compiled as C) references this as
// `extern uint8_t serialBuffer[]`.
extern "C" SerialBufferUnion serialBuffer;

void serialTxBufferClear();
bool usbTelemMirrorActive();

#endif

#endif // _SERIAL_BUFFER_UNION_H_
