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
 * Thin shim over stm32f0xx_ll_usb.c.
 *
 * Only the packet memory primitives are taken from the LL driver.  The
 * endpoint programming below is done with the direct register macros in
 * usb_ll.h rather than USB_ActivateEndpoint()/USB_EPStartXfer(), because the
 * LL endpoint API is built around the heavyweight USB_EPTypeDef bookkeeping
 * (xfer_len/xfer_count/xfer_len_db/xfer_fill_db) which we neither need nor can
 * afford in flash on this part.
 */

#include "usb_ll.h"
#include "stm32f0xx_ll_usb.h"

void usbPmaWrite(const uint8_t * src, uint16_t pmaAddr, uint16_t len)
{
  USB_WritePMA(USB, (uint8_t *)src, pmaAddr, len);
}

void usbPmaRead(uint8_t * dst, uint16_t pmaAddr, uint16_t len)
{
  USB_ReadPMA(USB, dst, pmaAddr, len);
}
