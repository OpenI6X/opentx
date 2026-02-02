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

#pragma once

#include "opentx.h"

typedef void (*DeleteFunc)(uint8_t idx);
typedef void (*InsertFunc)(uint8_t idx);
typedef void (*CopyFunc)(uint8_t idx);
typedef bool (*SwapFunc)(uint8_t & idx, uint8_t up);
typedef bool (*ReachLimitFunc)();
typedef void * (*AddressFunc)(uint8_t idx);
typedef uint8_t (*GetCountFunc)();
typedef void (*MenuCallback)(const char * result);
typedef void (*PushMenuFunc)(event_t);

struct CommonOps {
  DeleteFunc deleteFunc;
  InsertFunc insertFunc;
  CopyFunc copyFunc;
  SwapFunc swapFunc;
  ReachLimitFunc reachLimitFunc;
  AddressFunc addressFunc;
  GetCountFunc getCountFunc;
  MenuCallback menuCallback;
  PushMenuFunc pushMenuFunc;
  uint8_t maxItems;
  uint8_t maxChannels;
};

void onCommonMenu(const char * result, uint8_t chn, const CommonOps * ops);
void menuModelExposMixes_HandleEvent(event_t event, uint8_t chn, int8_t sub, const CommonOps * ops);
