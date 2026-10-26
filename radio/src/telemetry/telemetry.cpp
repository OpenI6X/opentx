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
#include "mixer_scheduler.h"

uint8_t telemetryStreaming = 0;
uint8_t telemetryRxBuffer[TELEMETRY_RX_PACKET_SIZE];   // Receive buffer. 9 bytes (full packet), worst case 18 bytes with byte-stuffing (+1)
uint8_t telemetryRxBufferCount = 0;

uint8_t telemetryState = TELEMETRY_INIT;

TelemetryData telemetryData;

uint8_t telemetryProtocol = 255;

static timer_handle_t telemetryTimer;

// Deferred frame processing (EdgeTX: async_call_isr(_poll_frame)): one-shot
// timer with period 0, i.e. run as soon as possible in the timer task.
static timer_handle_t _poll_frame_timer;
static void _poll_frame();

static void telemetryTimerCb()
{
 if (!s_pulses_paused) {
    DEBUG_TIMER_START(debugTimerTelemetryWakeup);
    telemetryWakeup();
    DEBUG_TIMER_STOP(debugTimerTelemetryWakeup);
 }
}

void telemetryStart()
{
  // before telemetryInit() / the A7105 start can enable a frame trigger
  if (!timer_is_created(&_poll_frame_timer)) {
    timer_create(&_poll_frame_timer, _poll_frame, 0, false);
  }

  if (!timer_is_created(&telemetryTimer)) {
    timer_create(&telemetryTimer, telemetryTimerCb, 2, true);
  }

  // clearTelemetryRxBuffers();
  timer_start(&telemetryTimer);
}


#if defined(TELEMETRY_STATS)
volatile uint32_t telemetryStatsRto;
volatile uint32_t telemetryStatsFrames;
#endif

static void telemetryMirrorSend(uint8_t data)
{
#if !defined(DEBUG) && defined(USB_SERIAL)
  if (usbTelemMirrorActive()) {
    usbSerialPutc(data);
  } else
#endif

#if defined(AUX_SERIAL)
  if (g_eeGeneral.auxSerialMode == UART_MODE_TELEMETRY_MIRROR) {
    auxSerialPutc(data);
  }
#endif
  {}
}

#if defined(AFHDS2A) && !defined(SIMU)
static void telemetryMirrorMPHeader(uint8_t type)
{
  telemetryMirrorSend('M');
  telemetryMirrorSend('P');
  telemetryMirrorSend(type);
  telemetryMirrorSend(AFHDS2A_RXPACKET_SIZE - 8);
}
#endif

// Deliberate difference to EdgeTX: no _telemetryIsPolling flag. In EdgeTX it
// stops pulsesStopModule() / a protocol change in pulsesSendNextFrame() from
// tearing down the module driver and serial port context (_deinit_module)
// while _poll_frame() is using them. Here nothing a module stop / protocol
// change does (intmoduleStop(), extmoduleStop(): module power and timers;
// moduleState; mixer period) is used by _poll_frame(), and the telemetry
// USART is only reconfigured by telemetryInit(), which runs in this same
// timer task and so cannot interleave with it. The flag would only delay
// protocol switches and cost ~100 B of flash.
static void _poll_frame()
{
#if defined(CROSSFIRE)
  if (telemetryProtocol == PROTOCOL_TELEMETRY_CROSSFIRE) {
    // EdgeTX copyRxBuffer() + processFrame(), with the bytes copied straight
    // behind the incomplete frame kept in telemetryRxBuffer (< 64 bytes, so
    // there is always room). Repeat if the FIFO held more than fitted.
    uint8_t frame_len;
    while ((frame_len = telemetryCopyRxBuffer(telemetryRxBuffer + telemetryRxBufferCount,
                                              TELEMETRY_RX_PACKET_SIZE - telemetryRxBufferCount)) > 0) {
      const uint8_t* frame = telemetryRxBuffer + telemetryRxBufferCount;
      LOG_TELEMETRY_WRITE_START();
      for (int i = 0; i < frame_len; i++) {
        telemetryMirrorSend(frame[i]);
        LOG_TELEMETRY_WRITE_BYTE(frame[i]);
      }
      crossfireProcessFrame(telemetryRxBuffer, &telemetryRxBufferCount, frame_len);
    }
  }
#endif
#if defined(AFHDS2A) && !defined(SIMU)
  // Only the A7105 ISR wakes the task in FlySky mode (after it has validated
  // the packet in telemetryRxBuffer). No ownership guard: the parse completes
  // long before the next radio packet (timer task priority, see tasksStart()).
  if (telemetryProtocol == PROTOCOL_TELEMETRY_FLYSKY_IBUS) {
    telemetryMirrorMPHeader(telemetryRxBuffer[0] == 0xAA ? 0x06 : 0x0C); // MPM telemetry types for AFHDS2A
    for (int i = 8; i < AFHDS2A_RXPACKET_SIZE; i++) {
      telemetryMirrorSend(telemetryRxBuffer[i]);
    }
    flyskyProcessFrame(telemetryRxBuffer);
  }
#endif
}

void telemetryFrameTrigger_ISR()
{
  timer_start(&_poll_frame_timer);
}

void processTelemetryData(uint8_t data)
{
#if defined(MULTIMODULE)
  if (telemetryProtocol == PROTOCOL_TELEMETRY_SPEKTRUM) {
    processSpektrumTelemetryData(data);
    return;
  }
  if (telemetryProtocol == PROTOCOL_TELEMETRY_FLYSKY_IBUS) {
    processFlySkyTelemetryData(data);
    return;
  }
  if (telemetryProtocol == PROTOCOL_TELEMETRY_MULTIMODULE) {
    processMultiTelemetryData(data);
    return;
  }
#endif
#if defined(TELEMETRY_FRSKY) && !defined(PCBI6X)
  processFrskyTelemetryData(data);
#endif
}

void telemetryWakeup()
{
  uint8_t requiredTelemetryProtocol = modelTelemetryProtocol();
  if (telemetryProtocol != requiredTelemetryProtocol) {
    telemetryInit(requiredTelemetryProtocol);
  }

  // CRSF / AFHDS2A frames are handled by _poll_frame(), triggered by the
  // USART receiver timeout / A7105 ISR: nothing to poll here.

  for (int i=0; i<MAX_TELEMETRY_SENSORS; i++) {
    const TelemetrySensor & sensor = g_model.telemetrySensors[i];
    if (sensor.type == TELEM_TYPE_CALCULATED) {
      telemetryItems[i].eval(sensor);
    }
  }

#if defined(VARIO)
  if (TELEMETRY_STREAMING() && !IS_FAI_ENABLED()) {
    varioWakeup();
  }
#endif
  
#define FRSKY_BAD_ANTENNA()            (IS_RAS_VALUE_VALID() && telemetryData.swr.value > 0x33)

  static tmr10ms_t alarmsCheckTime = 0;
  #define SCHEDULE_NEXT_ALARMS_CHECK(seconds) alarmsCheckTime = get_tmr10ms() + (100*(seconds))
  if (int32_t(get_tmr10ms() - alarmsCheckTime) > 0) {

    SCHEDULE_NEXT_ALARMS_CHECK(1/*second*/);

    bool sensor_lost = false;
    for (int i=0; i<MAX_TELEMETRY_SENSORS; i++) {
      if (isTelemetryFieldAvailable(i)) {
        TelemetryItem & item = telemetryItems[i];
        if (item.timeout == 0) {
          TelemetrySensor * sensor = & g_model.telemetrySensors[i];
          if (sensor->unit != UNIT_DATETIME) {
            item.setOld();
            sensor_lost = true;
          }
        }
      }
    }
    if (sensor_lost && TELEMETRY_STREAMING() &&  !g_model.disableTelemetryWarning) {
      audioEvent(AU_SENSOR_LOST);
    }

#if defined(PCBTARANIS) || defined(PCBHORUS)
    if ((isModulePXX(INTERNAL_MODULE) || isModulePXX(EXTERNAL_MODULE)) && FRSKY_BAD_ANTENNA()) {
      AUDIO_RAS_RED();
      POPUP_WARNING(STR_WARNING);
      const char * w = STR_ANTENNAPROBLEM;
      SET_WARNING_INFO(w, strlen(w), 0);
      SCHEDULE_NEXT_ALARMS_CHECK(10/*seconds*/);
    }
#endif

    if (!g_model.disableTelemetryWarning) {
      if (TELEMETRY_STREAMING()) {
        if (TELEMETRY_RSSI() < g_model.rfAlarms.critical ) {
          AUDIO_RSSI_RED();
          SCHEDULE_NEXT_ALARMS_CHECK(10/*seconds*/);
        }
        else if (TELEMETRY_RSSI() < g_model.rfAlarms.warning ) {
          AUDIO_RSSI_ORANGE();
          SCHEDULE_NEXT_ALARMS_CHECK(10/*seconds*/);
        }
      }

      if (TELEMETRY_STREAMING()) {
        if (telemetryState == TELEMETRY_KO) {
          AUDIO_TELEMETRY_BACK();
#if defined(CROSSFIRE)
          if (isModuleCrossfire(EXTERNAL_MODULE)) {
            moduleState[EXTERNAL_MODULE].counter = CRSF_FRAME_MODELID;
          }
#endif
        }
        telemetryState = TELEMETRY_OK;
      }
      else if (telemetryState == TELEMETRY_OK) {
        telemetryState = TELEMETRY_KO;
        AUDIO_TELEMETRY_LOST();
      }
    }
  }
}

void telemetryInterrupt10ms()
{
  if (TELEMETRY_STREAMING()) {
    if (!TELEMETRY_OPENXSENSOR()) {
      for (int i=0; i<MAX_TELEMETRY_SENSORS; i++) {
        const TelemetrySensor & sensor = g_model.telemetrySensors[i];
        if (sensor.type == TELEM_TYPE_CALCULATED) {
          telemetryItems[i].per10ms(sensor);
        }
      }
    }

  }

  if (telemetryStreaming > 0) {
    telemetryStreaming--;
  }
  else {
#if !defined(SIMU)
    telemetryData.rssi.reset();
#endif
  }
}

void telemetryReset()
{
  memclear(&telemetryData, sizeof(telemetryData));

  for (int index=0; index<MAX_TELEMETRY_SENSORS; index++) {
    telemetryItems[index].clear();
  }

  telemetryStreaming = 0; // reset counter only if valid frsky packets are being detected

  telemetryState = TELEMETRY_INIT;
}
// we don't reset the telemetry here as we would also reset the consumption after model load
void telemetryInit(uint8_t protocol)
{
  telemetryProtocol = protocol;
  switch(telemetryProtocol){
#if defined(TELEMETRY_FRSKY) && !defined(PCBI6X)
  case PROTOCOL_TELEMETRY_FRSKY_D:
	  telemetryPortInit(FRSKY_D_BAUDRATE, TELEMETRY_SERIAL_DEFAULT);
	  break;
#endif
#if defined(AUX_SERIAL) && !defined(PCBI6X)
  case PROTOCOL_TELEMETRY_FRSKY_D_SECONDARY:
	  telemetryPortInit(0, TELEMETRY_SERIAL_DEFAULT);
	  auxSerialTelemetryInit(PROTOCOL_TELEMETRY_FRSKY_D_SECONDARY);
  	  break;
#endif
#if defined(MULTIMODULE)
  case PROTOCOL_TELEMETRY_MULTIMODULE:
  case PROTOCOL_TELEMETRY_FLYSKY_IBUS:
	  // The DIY Multi module always speaks 100000 baud regardless of the telemetry protocol in use
	  telemetryPortInit(MULTIMODULE_BAUDRATE, TELEMETRY_SERIAL_8E2);
	  #if defined(LUA)
	  outputTelemetryBufferSize = 0;
	  outputTelemetryBufferTrigger = 0x7E;
	  #endif
	  break;
  case PROTOCOL_TELEMETRY_SPEKTRUM:
	  // Spektrum's own small race RX (SPM4648) uses 125000 8N1, use the same since there is no real standard
	  telemetryPortInit(125000, TELEMETRY_SERIAL_DEFAULT);
	  break;
#endif
#if defined(CROSSFIRE)
  case PROTOCOL_TELEMETRY_CROSSFIRE:
	  telemetryPortInit(CROSSFIRE_BAUDRATE, TELEMETRY_SERIAL_DEFAULT);

	  outputTelemetryBufferSize = 0;
	  outputTelemetryBufferTrigger = 0;

	  telemetryPortSetDirectionOutput();
	  break;
#endif
  default:
#if defined(TELEMETRY_FRSKY) && !defined(PCBI6X)
	  telemetryPortInit(FRSKY_SPORT_BAUDRATE, TELEMETRY_SERIAL_WITHOUT_DMA);
	  #if defined(LUA)
	  outputTelemetryBufferSize = 0;
	  outputTelemetryBufferTrigger = 0x7E;
      #endif
#endif
	  break;
  }
}


#if defined(LOG_TELEMETRY) && !defined(SIMU)
extern FIL g_telemetryFile;
void logTelemetryWriteStart()
{
  static tmr10ms_t lastTime = 0;
  tmr10ms_t newTime = get_tmr10ms();
  if (lastTime != newTime) {
    struct gtm utm;
    gettime(&utm);
    f_printf(&g_telemetryFile, "\r\n%4d-%02d-%02d,%02d:%02d:%02d.%02d0:", utm.tm_year+TM_YEAR_BASE, utm.tm_mon+1, utm.tm_mday, utm.tm_hour, utm.tm_min, utm.tm_sec, g_ms100);
    lastTime = newTime;
  }
}

void logTelemetryWriteByte(uint8_t data)
{
  f_printf(&g_telemetryFile, " %02X", data);
}
#endif

uint8_t outputTelemetryBuffer[TELEMETRY_OUTPUT_BUFFER_SIZE] __DMA;
uint8_t outputTelemetryBufferSize = 0;
uint8_t outputTelemetryBufferTrigger = 0;

#if defined(LUA)
Fifo<uint8_t, LUA_TELEMETRY_INPUT_FIFO_SIZE> * luaInputTelemetryFifo = NULL;
#endif

static ModuleSyncStatus moduleSyncStatus;

ModuleSyncStatus &getModuleSyncStatus(uint8_t moduleIdx)
{
  return moduleSyncStatus;
}

ModuleSyncStatus::ModuleSyncStatus()
{
  memset(this, 0, sizeof(ModuleSyncStatus));
}

void ModuleSyncStatus::update(uint16_t newRefreshRate, int16_t newInputLag)
{
  if (!newRefreshRate)
    return;
  
  if (newRefreshRate < MIN_REFRESH_RATE)
    newRefreshRate = newRefreshRate * (MIN_REFRESH_RATE / (newRefreshRate + 1));
  else if (newRefreshRate > MAX_REFRESH_RATE)
    newRefreshRate = MAX_REFRESH_RATE;

  refreshRate = newRefreshRate;
  inputLag    = newInputLag;
  currentLag  = newInputLag;
  lastUpdate  = get_tmr10ms();

  // TRACE("[SYNC] update rate = %dus; lag = %dus",refreshRate,currentLag);
}

uint16_t ModuleSyncStatus::getAdjustedRefreshRate()
{
  int16_t lag = currentLag;
  int32_t newRefreshRate = refreshRate;

  if (lag == 0) {
    return refreshRate;
  }
  
  newRefreshRate += lag;
  
  if (newRefreshRate < MIN_REFRESH_RATE) {
      newRefreshRate = MIN_REFRESH_RATE;
  }
  else if (newRefreshRate > MAX_REFRESH_RATE) {
    newRefreshRate = MAX_REFRESH_RATE;
  }

  currentLag -= newRefreshRate - refreshRate;
  // TRACE("[SYNC] mod rate = %dus; lag = %dus",newRefreshRate,currentLag);
  
  return (uint16_t)newRefreshRate;
}

