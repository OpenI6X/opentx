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

#include "model_inputs_mixes_common.h"

void onCommonMenu(const char * result, uint8_t chn, const CommonOps * ops)
{
  if (result == STR_EDIT) {
    pushMenu(ops->pushMenuFunc);
  }
  else if (result == STR_INSERT_BEFORE || result == STR_INSERT_AFTER) {
    if (!ops->reachLimitFunc()) {
      s_currCh = chn;
      if (result == STR_INSERT_AFTER) { s_currIdx++; menuVerticalPosition++; }
      ops->insertFunc(s_currIdx);
      pushMenu(ops->pushMenuFunc);
    }
  }
  else if (result == STR_COPY || result == STR_MOVE) {
    s_copyMode = (result == STR_COPY ? COPY_MODE : MOVE_MODE);
    s_copySrcIdx = s_currIdx;
    s_copySrcCh = chn;
    s_copySrcRow = menuVerticalPosition;
  }
  else if (result == STR_DELETE) {
    ops->deleteFunc(s_currIdx);
  }
}

void menuModelExposMixes_HandleEvent(event_t event, uint8_t chn, int8_t sub, const CommonOps * ops)
{
  switch (event) {
    case EVT_ENTRY:
    case EVT_ENTRY_UP:
      s_copyMode = 0;
      s_copyTgtOfs = 0;
      break;
    case EVT_KEY_LONG(KEY_EXIT):
      if (s_copyMode && s_copyTgtOfs == 0) {
        ops->deleteFunc(s_currIdx);
        killEvents(event);
        event = 0;
      }
      // no break
    case EVT_KEY_BREAK(KEY_EXIT):
      if (s_copyMode) {
        if (s_copyTgtOfs) {
          // cancel the current copy / move operation
          if (s_copyMode == COPY_MODE) {
            ops->deleteFunc(s_currIdx);
          }
          else {
            do {
              ops->swapFunc(s_currIdx, s_copyTgtOfs > 0);
              s_copyTgtOfs += (s_copyTgtOfs < 0 ? +1 : -1);
            } while (s_copyTgtOfs != 0);
            storageDirty(EE_MODEL);
          }
          menuVerticalPosition = s_copySrcRow + (ops->maxChannels == NUM_INPUTS ? 0 : HEADER_LINE);
          s_copyTgtOfs = 0;
        }
        s_copyMode = 0;
        event = 0;
      }
      break;
    case EVT_KEY_BREAK(KEY_ENTER):
      if ((!s_currCh || (s_copyMode && !s_copyTgtOfs)) && !READ_ONLY()) {
        s_copyMode = (s_copyMode == COPY_MODE ? MOVE_MODE : COPY_MODE);
        s_copySrcIdx = s_currIdx;
        s_copySrcCh = chn;
        s_copySrcRow = sub;
        break;
      }
      // no break
    case EVT_KEY_LONG(KEY_ENTER):
      killEvents(event);
      if (s_copyTgtOfs) {
        s_copyMode = 0;
        s_copyTgtOfs = 0;
      }
      else {
        if (READ_ONLY()) {
          if (!s_currCh) {
            pushMenu(ops->pushMenuFunc);
          }
        }
        else {
          if (s_copyMode) s_currCh = 0;
          if (s_currCh) {
            if (ops->reachLimitFunc()) break;
            ops->insertFunc(s_currIdx);
            pushMenu(ops->pushMenuFunc);
            s_copyMode = 0;
          }
          else {
            event = 0;
            s_copyMode = 0;
            POPUP_MENU_START(ops->menuCallback, 6, STR_EDIT, STR_INSERT_BEFORE, STR_INSERT_AFTER, STR_COPY, STR_MOVE, STR_DELETE);
          }
        }
      }
      break;
    case EVT_KEY_LONG(KEY_LEFT):
    case EVT_KEY_LONG(KEY_RIGHT):
      if (s_copyMode && !s_copyTgtOfs) {
        if (ops->reachLimitFunc()) break;
        s_currCh = chn;
        if (event == EVT_KEY_LONG(KEY_RIGHT)) { s_currIdx++; menuVerticalPosition++; }
        ops->insertFunc(s_currIdx);
        pushMenu(ops->pushMenuFunc);
        s_copyMode = 0;
        killEvents(event);
      }
      break;
    case EVT_KEY_FIRST(KEY_UP):
    case EVT_KEY_REPT(KEY_UP):
    case EVT_KEY_FIRST(KEY_DOWN):
    case EVT_KEY_REPT(KEY_DOWN):
#if defined(ROTARY_ENCODER_NAVIGATION)
    case EVT_ROTARY_RIGHT:
    case EVT_ROTARY_LEFT:
#endif
      if (s_copyMode) {
        uint8_t next_ofs = (IS_PREVIOUS_EVENT(event) ? s_copyTgtOfs - 1 : s_copyTgtOfs + 1);

        if (s_copyTgtOfs==0 && s_copyMode==COPY_MODE) {
          // insert a mix/expo on the same channel (just above / just below)
          if (ops->reachLimitFunc()) break;
          ops->copyFunc(s_currIdx);
          if (IS_NEXT_EVENT(event))
            s_currIdx++;
          else if (sub - menuVerticalOffset >= 6)
            menuVerticalOffset++;
        }
        else if (next_ofs==0 && s_copyMode==COPY_MODE) {
          // delete the mix/expo
          ops->deleteFunc(s_currIdx);
          if (IS_PREVIOUS_EVENT(event))
            s_currIdx--;
        }
        else {
          // only swap with its neighbor
          if (!ops->swapFunc(s_currIdx, IS_PREVIOUS_EVENT(event))) break;
          storageDirty(EE_MODEL);
        }

        s_copyTgtOfs = next_ofs;
      }
      break;
  }
}
