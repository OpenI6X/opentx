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

#include <inttypes.h>
#include "buzzer.h"

extern const char * warningText;
extern const char * warningInfoText;
extern uint8_t      warningInfoLength;
extern uint8_t      warningResult;
extern uint8_t      warningType;
extern uint8_t      warningInfoFlags;

#define MESSAGEBOX_X                   8
#define MESSAGEBOX_Y                   8
#define MESSAGEBOX_W                   (LCD_W - 15)

#define MENU_X                         MESSAGEBOX_X
#define MENU_Y                         MESSAGEBOX_Y
#define MENU_W                         MESSAGEBOX_W

#define WARNING_LINE_LEN               24
#define WARNING_LINE_X                 12
#define WARNING_LINE_Y                 MESSAGEBOX_Y + 2

#define POPUP_MENU_MAX_LINES         12
#define MENU_MAX_DISPLAY_LINES       6
#define MENU_LINE_LENGTH             (LEN_MODEL_NAME+12)

enum {
  MENU_OFFSET_INTERNAL,
  MENU_OFFSET_EXTERNAL
};

typedef void         (* PopupFunc)(event_t event);
extern PopupFunc popupFunc;
extern uint8_t popupMenuOffsetType;

extern uint16_t popupMenuOffset;
extern const char * popupMenuItems[POPUP_MENU_MAX_LINES];
extern uint16_t popupMenuItemsCount;
typedef void         (* PopupMenuHandler)(const char * result);
extern PopupMenuHandler popupMenuHandler;
extern const char * popupMenuTitle;
extern uint8_t popupMenuSelectedItem;

// Message box
void drawMessageBoxBackground(coord_t top, coord_t height);
void drawMessageBox();
void showMessageBox(const char * title);

// Popup menu
const char * runPopupMenu(event_t event);
void runPopupWarning(event_t event);

enum
{
  WARNING_TYPE_WAIT,
  WARNING_TYPE_INFO,
  WARNING_TYPE_ASTERISK,
  WARNING_TYPE_CONFIRM,
  WARNING_TYPE_INPUT
};

#if !defined(GUI)
  #define DISPLAY_WARNING(...)
  #define POPUP_WARNING(...)
  #define POPUP_CONFIRMATION(...)
  #define POPUP_INPUT(...)
  #define SET_WARNING_INFO(...)
#else
  #define DISPLAY_WARNING(evt)              (*popupFunc)(evt)
  extern void POPUP_INFORMATION(const char * s);
  extern void POPUP_WARNING(const char * message/*, const char * info = nullptr, bool waitForClose = true*/);
  extern void SET_WARNING_INFO(const char * info, uint8_t length, uint8_t flags);
  extern void POPUP_CONFIRMATION(const char * s/*, PopupMenuHandler handler*/);
  extern void POPUP_INPUT(const char * s, PopupFunc func);
  extern void POPUP_MENU_ADD_ITEM(const char * s);
  extern bool isEventCaughtByPopup();
  extern void POPUP_MENU_TITLE(const char * s);
  extern void POPUP_MENU_START(PopupMenuHandler handler);
  extern void POPUP_MENU_START(PopupMenuHandler handler, int count, ...);
#endif


#if defined(SDCARD)
  #define POPUP_MENU_ADD_SD_ITEM(s)    POPUP_MENU_ADD_ITEM(s)
#else
  #define POPUP_MENU_ADD_SD_ITEM(s)
#endif
