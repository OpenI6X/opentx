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

#include "opentx.h"

constexpr coord_t CHANNEL_NAME_OFFSET = 0;
constexpr coord_t CHANNEL_VALUE_OFFSET = CHANNEL_NAME_OFFSET + 43;
constexpr coord_t CHANNEL_GAUGE_OFFSET = CHANNEL_VALUE_OFFSET - 1;
constexpr coord_t CHANNEL_BAR_WIDTH = 70;
constexpr coord_t CHANNEL_PROPERTIES_OFFSET = CHANNEL_GAUGE_OFFSET + CHANNEL_BAR_WIDTH + 2;

#if defined(NAVIGATION_X7)
#define EVT_KEY_PREVIOUS_VIEW          EVT_KEY_LONG(KEY_PAGE)
#define EVT_KEY_NEXT_VIEW              EVT_KEY_BREAK(KEY_PAGE)
#define EVT_KEY_NEXT_PAGE              EVT_ROTARY_RIGHT
#define EVT_KEY_PREVIOUS_PAGE          EVT_ROTARY_LEFT
#elif defined(NAVIGATION_XLITE)
#define EVT_KEY_PREVIOUS_VIEW          EVT_KEY_BREAK(KEY_UP)
#define EVT_KEY_NEXT_VIEW              EVT_KEY_BREAK(KEY_DOWN)
#define EVT_KEY_NEXT_PAGE              EVT_KEY_BREAK(KEY_RIGHT)
#define EVT_KEY_PREVIOUS_PAGE          EVT_KEY_BREAK(KEY_LEFT)
#else
#define EVT_KEY_PREVIOUS_VIEW          EVT_KEY_BREAK(KEY_UP)
#define EVT_KEY_NEXT_VIEW              EVT_KEY_BREAK(KEY_DOWN)
#define EVT_KEY_NEXT_PAGE              EVT_KEY_BREAK(KEY_RIGHT)
#define EVT_KEY_PREVIOUS_PAGE          EVT_KEY_BREAK(KEY_LEFT)
#endif

static bool mixersView = false;

void menuChannelsViewCommon(event_t event)
{
  uint8_t ch;

  switch (event) {
    case EVT_KEY_FIRST(KEY_ENTER):
      mixersView = !mixersView;
      break;
  }

  ch = 8 * (g_eeGeneral.view / ALTERNATE_VIEW);

  // Screen title
  lcdDrawText(LCD_W / 2, 0, mixersView ? STR_MIXERS_MONITOR : STR_CHANNELS_MONITOR, CENTERED);
  lcdInvertLine(0);

  int16_t limits = 512 * 2;

// Channels
  for (uint32_t line = 0; line < 8; line++) {
    LimitData * ld = limitAddress(ch);
    const uint8_t y = 9 + line * 7;
    const int32_t val = mixersView ? ex_chans[ch] : channelOutputs[ch];
    const uint8_t lenLabel = ZLEN(g_model.limitData[ch].name);

    // Channel name if present, number if not
    if (lenLabel > 0) {
      lcdDrawSizedText(CHANNEL_NAME_OFFSET, y, g_model.limitData[ch].name, sizeof(g_model.limitData[ch].name), ZCHAR | SMLSIZE);
    }
    else {
      putsChn(CHANNEL_NAME_OFFSET, y, ch + 1, SMLSIZE);
    }

    // Value
    drawChannelValue(CHANNEL_VALUE_OFFSET, y + 1, ch, val, TINSIZE | RIGHT);

    // Gauge
    drawGauge(CHANNEL_GAUGE_OFFSET, y, CHANNEL_BAR_WIDTH, 6, val, limits);

    if (!mixersView) {
      // Properties
#if defined(OVERRIDE_CHANNEL_FUNCTION)
      if (safetyCh[ch] != OVERRIDE_CHANNEL_UNDEFINED)
        lcdDrawText(CHANNEL_PROPERTIES_OFFSET, y, "OVR", TINSIZE);
      else
#endif
      if (ld && ld->revert) {
        lcdDrawText(CHANNEL_PROPERTIES_OFFSET, y, "INV", TINSIZE);
      }
    }

    ++ch;
  }
}

void menuChannelsView(event_t event)
{
  switch (event) {
    case EVT_KEY_BREAK(KEY_EXIT):
      popMenu();
      break;

    case EVT_KEY_NEXT_PAGE:
      g_eeGeneral.view = (g_eeGeneral.view + (4 * ALTERNATE_VIEW) + ALTERNATE_VIEW) % (4 * ALTERNATE_VIEW);
      break;

    case EVT_KEY_PREVIOUS_PAGE:
      g_eeGeneral.view = (g_eeGeneral.view + (4 * ALTERNATE_VIEW) - ALTERNATE_VIEW) % (4 * ALTERNATE_VIEW);
      break;
  }

  menuChannelsViewCommon(event);
}