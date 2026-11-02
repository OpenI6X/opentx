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

#define STATS_1ST_COLUMN               1
#define STATS_2ND_COLUMN               7*FW+FW/2
#define STATS_3RD_COLUMN               14*FW+FW/2
#define STATS_LABEL_WIDTH              3*FW

void menuStatisticsView(event_t event)
{
  TITLE(STR_MENUSTAT);

  switch (event) {
    case EVT_KEY_FIRST(KEY_UP):
#if defined(PCBX7)
    case EVT_KEY_BREAK(KEY_PAGE):
#endif
      chainMenu(menuStatisticsDebug);
      break;

    case EVT_KEY_FIRST(KEY_DOWN):
#if defined(PCBX7)
    case EVT_KEY_LONG(KEY_PAGE):
      killEvents(event);
      chainMenu(menuStatisticsDebug2);
#else
      chainMenu(menuStatisticsDebug);
#endif
      break;

#if !defined(PCBTARANIS)
    case EVT_KEY_LONG(KEY_MENU): // historical
#endif
#if !defined(PCBI6X)
    case EVT_KEY_LONG(KEY_ENTER):
#endif
      g_eeGeneral.globalTimer = 0;
      storageDirty(EE_GENERAL);
      sessionTimer = 0;
      break;
    case EVT_KEY_FIRST(KEY_EXIT):
      chainMenu(menuMainView);
      break;
  }

  // Session and Total timers
  lcdDrawText(STATS_1ST_COLUMN, FH*1+1, "SES", BOLD);
  drawTimer(STATS_1ST_COLUMN + STATS_LABEL_WIDTH, FH*1+1, sessionTimer, 0, 0);
  lcdDrawText(STATS_1ST_COLUMN, FH*2+1, "TOT", BOLD);
  drawTimer(STATS_1ST_COLUMN + STATS_LABEL_WIDTH, FH*2+1, g_eeGeneral.globalTimer + sessionTimer, TIMEHOUR, 0);

  // Throttle special timers
  lcdDrawText(STATS_2ND_COLUMN, FH*0+1, "THR", BOLD);
  drawTimer(STATS_2ND_COLUMN + STATS_LABEL_WIDTH, FH*0+1, s_timeCumThr, 0, 0);
  lcdDrawText(STATS_2ND_COLUMN, FH*1+1, "TH%", BOLD);
  drawTimer(STATS_2ND_COLUMN + STATS_LABEL_WIDTH, FH*1+1, s_timeCum16ThrP/16, 0, 0);

  // Timers
  for (int i=0; i<TIMERS; i++) {
    drawStringWithIndex(STATS_3RD_COLUMN, FH*i+1, "TM", i+1, BOLD);
    if (timersStates[i].val > 3600)
      drawTimer(STATS_3RD_COLUMN + STATS_LABEL_WIDTH, FH*i+1, timersStates[i].val, TIMEHOUR, 0);
    else
      drawTimer(STATS_3RD_COLUMN + STATS_LABEL_WIDTH, FH*i+1, timersStates[i].val, 0, 0);
  }

#if defined(THRTRACE)
  const coord_t x = 5;
  const coord_t y = 60;
  lcdDrawSolidHorizontalLine(x-3, y, MAXTRACE+3+3);
  lcdDrawSolidVerticalLine(x, y-32, 32+3);
  for (coord_t i=0; i<MAXTRACE; i+=6) {
    lcdDrawSolidVerticalLine(x+i+6, y-1, 3);
  }

  uint16_t traceRd = s_traceWr > MAXTRACE ? s_traceWr - MAXTRACE : 0;
  for (coord_t i=1; i<=MAXTRACE && traceRd<s_traceWr; i++, traceRd++) {
    uint8_t h = s_traceBuf[traceRd % MAXTRACE];
    lcdDrawSolidVerticalLine(x+i, y-h, h);
  }
#endif
}

#define MENU_DEBUG_COL1_OFS          (11*FW-3)
#define MENU_DEBUG_COL2_OFS          (17*FW)

void menuStatisticsDebug(event_t event)
{
  TITLE(STR_MENUDEBUG);

  switch (event) {
    case EVT_KEY_LONG(KEY_ENTER):
      g_eeGeneral.globalTimer = 0;
      sessionTimer = 0;
      storageDirty(EE_GENERAL);
      killEvents(event);
      break;

    case EVT_KEY_FIRST(KEY_ENTER):
    //   telemetryErrors = 0;
      maxMixerDuration = 0;
#if defined(TELEMETRY_STATS)
      telemetryStatsRto = 0;
      telemetryStatsFrames = 0;
#endif
      break;


    case EVT_KEY_FIRST(KEY_UP):
#if defined(PCBX7)
    case EVT_KEY_BREAK(KEY_PAGE):
#endif
#if !defined(PCBI6X) // single debug screen
      chainMenu(menuStatisticsDebug2);
      return;
#endif

    case EVT_KEY_FIRST(KEY_DOWN):
#if defined(PCBX7)
    case EVT_KEY_LONG(KEY_PAGE):
#endif
      killEvents(event);
      chainMenu(menuStatisticsView);
      break;

    case EVT_KEY_FIRST(KEY_EXIT):
      chainMenu(menuMainView);
      break;
  }

  uint8_t y = FH + 1;

#if defined(PCBI6X) && defined(TLM_ERRORS) // single debug screen
  lcdDrawTextAlignedLeft(y, "Tlm RX Err");
  lcdDrawNumber(MENU_DEBUG_COL1_OFS + FW, y, telemetryErrors, RIGHT);
  y += FH;
#endif

#if defined(PCBTARANIS) //|| defined(PCBI6X)
#if !defined(SIMU) && defined(DEBUG)
  lcdDrawTextAlignedLeft(y, "Usb");
  lcdDrawNumber(MENU_DEBUG_COL1_OFS, y, charsWritten, LEFT);
  lcdDrawText(lcdLastRightPos, y, " ");
  lcdDrawNumber(lcdLastRightPos, y, APP_Tx_ptr_in, LEFT);
  lcdDrawText(lcdLastRightPos, y, " ");
  lcdDrawNumber(lcdLastRightPos, y, APP_Rx_ptr_out, LEFT);
  lcdDrawText(lcdLastRightPos, y, " ");
  lcdDrawNumber(lcdLastRightPos, y, usbWraps, LEFT);
  y += FH;
#endif
  lcdDrawTextAlignedLeft(y, "Free Mem");
  lcdDrawNumber(MENU_DEBUG_COL1_OFS, y, availableMemory(), LEFT);
  lcdDrawText(lcdLastRightPos, y, "b");
  y += FH;
#endif // PCBTARANIS

//  lcdDrawTextAlignedLeft(y, "Stack");
//  lcdDrawNumber(MENU_DEBUG_COL1_OFS, y, stackAvailable(), LEFT);
//  lcdDrawText(lcdLastRightPos, y, "/");
//  lcdDrawNumber(lcdLastRightPos, y, stackSize() * 4, LEFT);
//  y += FH;

//  lcdDrawTextAlignedLeft(y, "USB Conn");
//  lcdDrawNumber(MENU_DEBUG_COL1_OFS, y, ((GPIOA->IDR & GPIO_IDR_11) ? 1 : 0), LEFT);
//  y += FH;

  lcdDrawTextAlignedLeft(y, STR_TMIXMAXMS);
  lcdDrawNumber(MENU_DEBUG_COL1_OFS, y, DURATION_MS_PREC2(maxMixerDuration), PREC2|LEFT);
  lcdDrawText(lcdLastRightPos, y, "ms");
  y += FH;

  lcdDrawTextAlignedLeft(y, STR_FREESTACKMINB);
  lcdDrawNumber(MENU_DEBUG_COL1_OFS, y, task_get_stack_usage(&menusTaskId), LEFT);
  lcdDrawText(lcdLastRightPos, y, "/");
  lcdDrawNumber(lcdLastRightPos, y, task_get_stack_usage(&mixerTaskId), LEFT);
  lcdDrawText(lcdLastRightPos, y, "/");
  lcdDrawNumber(lcdLastRightPos, y, task_get_stack_usage(&timerTaskId), LEFT);

#if defined(TASK_CPU_STATS)
  y += FH;
  {
    // 1 s snapshot over the idle / task CPU time counters
    const uint32_t winMs = 1000u;
    static uint32_t lastTotal = 0, lastIdle = 0;
    static uint16_t cpuPc = 0, idleMs = 0;
    // same order as the stack line: menus / mixer / timer
    task_handle_t * const tasks[3] = {&menusTaskId, &mixerTaskId, &timerTaskId};
    static uint32_t lastTask[3];
    static uint16_t taskPm[3];         // per mille of the window
    uint32_t total = time_get_ms();
    uint32_t idle = task_get_idle_time();
    uint32_t dTotal = total - lastTotal;
    if (dTotal >= winMs) {
      bool stale = dTotal > 10u * winMs;
      uint32_t unitsPerMille = dTotal * TASK_CPU_UNITS_PER_MS / 1000u;
      for (uint8_t i = 0; i < 3; i++) {
        uint32_t t = task_get_cpu_time(tasks[i]);
        if (!stale) {
          uint32_t pm = (t - lastTask[i]) / unitsPerMille;
          taskPm[i] = (uint16_t)(pm > 1000 ? 1000 : pm);
        }
        lastTask[i] = t;
      }
      if (stale) {
        // stale baseline (screen just opened): reseed, skip this round
        lastTotal = total;
        lastIdle = idle;
      }
      else {
        uint32_t dIdle = (idle - lastIdle) / TASK_CPU_UNITS_PER_MS;
        if (dIdle > dTotal)
          dIdle = dTotal;                      // tick raced the two reads
        idleMs = (uint16_t)dIdle;
        cpuPc = (uint16_t)(100u - (100u * dIdle) / dTotal);
        lastTotal = total;
        lastIdle = idle;
      }
    }
    lcdDrawTextAlignedLeft(y, "CPU");
    lcdDrawNumber(MENU_DEBUG_COL1_OFS, y, cpuPc, LEFT);
    lcdDrawText(lcdLastRightPos, y, "% ");
    lcdDrawNumber(lcdLastRightPos, y, idleMs, LEFT);
    lcdDrawText(lcdLastRightPos, y, "ms");
//#if defined(RTOS_HAS_TASK_STATS)
    // CPU % per task, 0.1 % resolution (ISR time goes to the interrupted task)
    y += FH;
    lcdDrawTextAlignedLeft(y, "Task%");
    lcdDrawNumber(MENU_DEBUG_COL1_OFS, y, taskPm[0], PREC1|LEFT);
    for (uint8_t i = 1; i < 3; i++) {
      lcdDrawText(lcdLastRightPos, y, "/");
      lcdDrawNumber(lcdLastRightPos, y, taskPm[i], PREC1|LEFT);
    }
//#endif
  }
#endif

#if defined(TELEMETRY_STATS)
  // USART receiver timeouts, then frames parsed OK.
  // Expected with RTO working (CRSF): RTO ~= Frm (+ junk bursts).
  y += FH;
  lcdDrawTextAlignedLeft(y, "Tlm RTO");
  lcdDrawNumber(MENU_DEBUG_COL1_OFS, y, telemetryStatsRto, LEFT);
  y += FH;
  lcdDrawTextAlignedLeft(y, "Tlm Frm");
  lcdDrawNumber(MENU_DEBUG_COL1_OFS, y, telemetryStatsFrames, LEFT);
#endif

  lcdDrawText(5*FW, 7*FH+1, STR_MENUTORESET);
  lcdInvertLastLine();
}

#if defined(STM32) && !defined(PCBI6X)
void menuStatisticsDebug2(event_t event)
{
  TITLE(STR_MENUDEBUG);

  switch (event) {
    case EVT_KEY_FIRST(KEY_ENTER):
    //   telemetryErrors  = 0;
      break;

    case EVT_KEY_FIRST(KEY_UP):
#if defined(PCBX7)
    case EVT_KEY_BREAK(KEY_PAGE):
#endif
      chainMenu(menuStatisticsView);
      return;

    case EVT_KEY_FIRST(KEY_DOWN):
#if defined(PCBX7)
    case EVT_KEY_LONG(KEY_PAGE):
#endif
      killEvents(event);
      chainMenu(menuStatisticsDebug);
      break;

    case EVT_KEY_FIRST(KEY_EXIT):
      chainMenu(menuMainView);
      break;
  }

  lcdDrawText(4*FW, 7*FH+1, STR_MENUTORESET);
  lcdInvertLastLine();
}
#endif
