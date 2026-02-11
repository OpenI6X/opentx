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
#include "opentx.h"

void hello_draw() {
  lcdDrawLine(10, 10, 110, 25);
  lcdDrawText((LCD_W - (FW * 10)) / 2, (LCD_H - FH) / 2, "Hello World", BOLD);
  lcdDrawText((LCD_W - (FW * 10)) / 2, ((LCD_H - FH) / 2) + 10, "RSSI: ", TINSIZE);
  lcdDrawNumber(lcdLastRightPos, ((LCD_H - FH) / 2) + 10, telemetryData.rssi.value, TINSIZE);

  uint8_t sensorsCount = 0;
  for (int i=0; i<MAX_TELEMETRY_SENSORS; i++) {
    if (isTelemetryFieldAvailable(i)) {
//      TelemetrySensor & sensor = g_model.telemetrySensors[i];
      sensorsCount++;
    }
  }

  lcdDrawText((LCD_W - (FW * 10)) / 2, ((LCD_H - FH) / 2) + 20, "Sensors: ", TINSIZE);
  lcdDrawNumber(lcdLastRightPos, LCD_H/2 - (FH/2) + 20, sensorsCount, TINSIZE);
}

void hello_stop() {
  if (globalData.cToolRunning) {
    globalData.cToolRunning = 0;
    popMenu();
  }
}

void hello_run(event_t event) {
  if (globalData.cToolRunning == 0) {
    globalData.cToolRunning = 1;
  }

  if (event == EVT_KEY_LONG(KEY_EXIT)) { // exit on LONG press CANCEL
    hello_stop();
  }

  // run every frame
  lcdClear();
  hello_draw();
}
