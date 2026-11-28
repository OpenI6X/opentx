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

#ifndef _OTXTYPES_H_
#define _OTXTYPES_H_

#include <inttypes.h>

typedef uint32_t tmr10ms_t;
typedef int32_t rotenc_t;
typedef int32_t getvalue_t;
typedef uint32_t mixsrc_t;
typedef int32_t swsrc_t;
typedef int16_t safetych_t;
typedef uint32_t bitfield_channels_t;

// Text parameters/variables that may receive a TRZ-compressed STR_ string
// (radio/util/trz.py reads these marks; both are plain const char *):
//   trz_text      the text is only ever drawn (lcdDrawSizedText()/getTextWidth()
//                 decode it) or passed on to another trz_text
//   trz_item      the same, but the pointer is also kept and compared later
//                 (popup menu items and results)
//   TRZ_VA_ITEMS  in front of "...": every variadic argument is a trz_item
// Function-like macros need no mark: trz.py follows them to the call they expand to.
typedef const char * trz_text;
typedef const char * trz_item;
#define TRZ_VA_ITEMS

#endif // _OTXTYPES_H_
